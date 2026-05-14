# cbang Async DNS

cbang ships an async DNS resolver that runs on the event loop, caches
responses with TTL, retries on failure, supports multiple
nameservers, and bypasses DNS entirely when given a literal IP
address.  Most users never use it directly — `cb::HTTP::Client`,
`cb::Event::Connection`, and other networking code call it
transparently.

## Concepts

- **`cb::DNS::Base`** — the resolver.  One per `cb::Event::Base`;
  accessed via `base.getDNS()`.
- **`cb::DNS::Request`** — an in-flight query.  Returned by
  `resolve()` / `reverse()`; cancel by calling `.cancel()` or by
  letting the SmartPointer go.
- **`cb::DNS::Result`** — the response.  Carries an `Error` and a
  vector of `SockAddr` (forward lookup) or `string` (reverse).
- **`cb::DNS::Error`** — enum: `DNS_ERR_NOERROR`,
  `DNS_ERR_NOTEXIST`, `DNS_ERR_SERVERFAILED`, etc.
- **`cb::DNS::Type`** — record type: `DNS_A`/`DNS_IPV4`,
  `DNS_AAAA`/`DNS_IPV6`, `DNS_PTR` for reverse, and the rest of the
  standard DNS types if you build raw `Request`s.

The resolver is fully event-driven: no thread for DNS work, no
synchronous blocking.  Callbacks fire on the loop thread.

## Setup

If you have a `cb::Event::Base`, you already have a DNS resolver:

```cpp
#include <cbang/event/Base.h>
#include <cbang/dns/Base.h>

cb::Event::Base base;
cb::DNS::Base  &dns = base.getDNS();   // lazily created on first call

dns.initSystemNameservers();           // load /etc/resolv.conf
```

`initSystemNameservers()` populates the resolver from the system
config (Unix `resolv.conf`, Windows registry).  Without it, you
won't have any nameservers; add them manually with
`addNameserver(addr)`.

The HTTP client calls `initSystemNameservers` on demand, so for most
applications you never touch the DNS object directly.

## Forward lookup

```cpp
auto cb = [] (cb::DNS::Error err, const std::vector<cb::SockAddr> &addrs) {
  if (err) { LOG_WARNING("DNS: " << err); return; }
  for (auto &a: addrs) LOG_INFO(1, "addr=" << a);
};

auto req = dns.resolve("example.com", cb);
// req is a SmartPointer<DNS::Request>.  Keep it alive until the
// callback fires, or release it to cancel.
```

The callback runs on the loop thread when:
- The lookup succeeds (one or more addresses).
- The lookup fails (timeout, server error, NXDOMAIN).
- The request is cancelled (callback is dropped; cancel is silent).

Pass `ipv6 = true` for an AAAA lookup:

```cpp
auto req = dns.resolve("example.com", cb, /*ipv6*/ true);
```

### IP-literal fast path

If the "hostname" is already an IP literal (`"127.0.0.1"`,
`"::1"`), `resolve()` skips the DNS pathway entirely and invokes
the callback synchronously with the parsed `SockAddr`.  This is how
`cb::Event::Connection::connect` accepts hostnames or literals
uniformly:

```cpp
// Both work — no special-casing required.
dns.resolve("example.com", cb);
dns.resolve("203.0.113.42",  cb);
```

## Reverse lookup

```cpp
cb::SockAddr addr = cb::SockAddr::parseIPv4("8.8.8.8");

auto cb = [] (cb::DNS::Error err, const std::vector<std::string> &names) {
  if (err) { LOG_WARNING("DNS: " << err); return; }
  for (auto &n: names) LOG_INFO(1, "name=" << n);
};

auto req = dns.reverse(addr, cb);
```

There's also a string overload that parses the address for you.

## Cancelling

```cpp
auto req = dns.resolve("example.com", cb);
req->cancel();          // explicit
req.release();          // drop the strong reference — cancels if last
```

After `cancel()`, the callback will not fire (the resolver tracks
canceled requests and skips their callbacks).

## Caching

The resolver caches responses by name, using the DNS TTL as the
expiry.  Repeated lookups within the TTL window return cached
results synchronously (the callback still runs on the loop, just
without a network round-trip).

The cache is shared across all callers of the same `DNS::Base`.

## Configuration

The `DNS::Base` has several tunables, all settable from your
application:

| Setter | Default | Meaning |
|---|---|---|
| `setMaxActive(n)` | 512 | Cap on concurrent queries. |
| `setQueryTimeout(s)` | 5 | Per-nameserver query timeout. |
| `setRequestTimeout(s)` | 16 | Total time including retries. |
| `setMaxAttempts(n)` | 3 | Retries per nameserver before failing over. |
| `setMaxFailures(n)` | 16 | Failures before a nameserver is dropped. |
| `setBindAddress(addr)` | `0.0.0.0` | Source bind for DNS sockets. |

Add nameservers manually if `initSystemNameservers()` doesn't fit
your needs:

```cpp
dns.addNameserver("8.8.8.8");
dns.addNameserver("1.1.1.1");
```

## Errors

The `cb::DNS::Error` enum values you'll typically see:

| Value | Meaning |
|---|---|
| `DNS_ERR_NOERROR` | Success. |
| `DNS_ERR_NOTEXIST` | NXDOMAIN — name does not exist. |
| `DNS_ERR_SERVERFAILED` | Upstream server returned SERVFAIL. |
| `DNS_ERR_TIMEOUT` | All retries exhausted. |
| `DNS_ERR_FORMAT` | Malformed response. |
| `DNS_ERR_REFUSED` | Server refused. |

The result is always provided; on error, the address list is empty.
Treat any non-`DNS_ERR_NOERROR` as a transport failure and retry or
fall through to the user.

## Integration with HTTP

`cb::Event::Connection::connect(hostname, port, bind)` calls
`base.getDNS().resolve(hostname, ...)` internally.  The HTTP client
inherits the same caching, retry, and IP-literal fast path.  You'd
only interact with `DNS::Base` directly if you need:

- Reverse lookups.
- Bulk pre-resolution.
- Custom nameserver lists separate from system defaults.
- TXT/MX/SRV records (use a raw `cb::DNS::Request` subclass).

## Patterns

### Pre-resolve a list

```cpp
for (auto &name: hostnames) {
  dns.resolve(name, [name] (DNS::Error e, const auto &addrs) {
    if (!e && !addrs.empty()) cache[name] = addrs.front();
  });
}
```

The cache field doubles as both your application-level cache and the
resolver's TTL-bound one.

### Cancel on shutdown

```cpp
class Worker : public cb::RefCounted {
  cb::DNS::Base::RequestPtr req;
public:
  void start(cb::DNS::Base &dns) {
    req = dns.resolve("example.com", WeakCall(this, &Worker::onDNS));
  }
  void onDNS(DNS::Error e, const std::vector<SockAddr> &addrs) { /* ... */ }
  ~Worker() { req.release(); }   // cancels if pending
};
```

`WeakCall` lets the callback safely no-op if the Worker dies before
the lookup returns; the explicit `release()` in the destructor
cancels the in-flight request.

### Forcing a refresh

The resolver respects DNS TTLs.  If you need to bypass the cache,
either wait out the TTL or restart the `DNS::Base`.

## Common pitfalls

- **Forgetting `initSystemNameservers()`.**  With no nameservers
  configured, every lookup fails with no upstream.  HTTP clients do
  this for you; standalone DNS users must call it.
- **Letting the `RequestPtr` go out of scope mid-flight.**  Same as
  every other SmartPointer-managed resource: drop the strong
  reference, the request is cancelled.  Store it on an instance if
  you need it to outlive the local scope.
- **Treating the IP-literal fast path as synchronous in your
  control flow.**  It IS synchronous in that the callback fires
  before `resolve()` returns, but your code still needs to handle
  the async-shaped path for real hostnames.
- **Assuming address ordering.**  Order varies between calls; for
  load balancing or affinity, sort or randomize yourself.

## See also

- `cbang/dns/Base.h`, `cbang/dns/Request.h`, `cbang/dns/Result.h`,
  `cbang/dns/Error.h`, `cbang/dns/Type.h`.
- [EventSystem.md](EventSystem.md) — the loop that drives DNS.
- [WebServer.md](WebServer.md) — the HTTP layer that uses DNS
  transparently.
