# cbang Net Module

This document covers `cbang/net/` — the low-level networking primitives that
sit beneath the event-driven HTTP/HTTPS/WebSocket stack:

  - `SockAddr` — IPv4/IPv6/Unix socket address (parse, format, compare)
  - `Socket` / `SocketSet` — blocking BSD-style socket wrapper
  - `URI` — RFC 3986 URI parse/build with query-string accessors
  - `AddressRange`, `AddressRangeSet`, `AddressFilter` — CIDR / range
    matching and allow/deny lists
  - `Base64`, `URLBase64` — Base64 codec
  - `Swab.h`, `Winsock.h`, `SocketType.h` — portability helpers

Most application code uses the asynchronous stack
([WebServer.md](WebServer.md), [EventSystem.md](EventSystem.md)) and only
touches this module indirectly.  The blocking `Socket` is rarely the right
choice in a libevent-based program; the value of the net module for
application code is mostly `URI`, `SockAddr`, and the address-filter classes.

## Headers

```cpp
#include <cbang/net/SockAddr.h>
#include <cbang/net/Socket.h>
#include <cbang/net/SocketSet.h>
#include <cbang/net/URI.h>
#include <cbang/net/AddressRange.h>
#include <cbang/net/AddressRangeSet.h>
#include <cbang/net/AddressFilter.h>
#include <cbang/net/Base64.h>
```

## SockAddr

`cb::SockAddr` is a uniform address container that hides the differences
between `sockaddr_in`, `sockaddr_in6`, and `sockaddr_un`.  Construct from a
parsed string, a raw `sockaddr`, an IPv4 integer + port, or a Unix path:

```cpp
SockAddr a("127.0.0.1:8080");
SockAddr b("[::1]:8080");
SockAddr u("/var/run/foo.sock");          // Unix domain
SockAddr c(0x7F000001, 8080);             // IPv4 from uint32_t

// Parse forms (throw on failure)
auto x = SockAddr::parse("10.0.0.1/24");      // returns SockAddr
auto y = SockAddr::parseIPv6("fe80::1");
```

Inspection:

```cpp
a.isIPv4();  a.isIPv6();  a.isUnix();  a.isLoopback();
a.getPort();
a.getIPv4();                 // uint32_t, host byte order
a.getIPv6();                 // const uint8_t* (16 bytes)
a.getPath();                 // Unix path
a.toString();                // "127.0.0.1:8080" or "[::1]:8080" or "/path"
a.toString(false);           // address only, no port
```

Comparison and ordering: all standard relational operators, plus
`cmp(other, /*cmpPorts=*/true)` if you want to compare addresses ignoring
port.  `SockAddr` is hashable via its byte representation and works fine as a
`std::map` key.

CIDR support is built in:

```cpp
SockAddr lo("10.1.2.0");
lo.setCIDRBits(24, false);   // zero everything past bit 24 (network base)
int8_t bits = lo.getCIDRBits(highEnd);
```

`getCIDRBits(end)` returns the prefix length covering `[this, end]` as a
single CIDR block, or `-1` if the range can't be expressed that way.

Predicates for raw strings (no exception, returns bool):

```cpp
SockAddr::isIPv4Address("10.0.0.1");
SockAddr::isIPv6Address("::1");
SockAddr::isUnixAddress("/var/run/x");
SockAddr::isAddress(s);
```

Low-level bind/connect/accept methods (`bind(socket_t)`, `connect(socket_t)`,
`accept(socket_t)`) are present but generally only used by `Socket` itself or
by the libevent integration in `cbang/event/`.

## Socket

`cb::Socket` is a thin blocking wrapper around a BSD socket.  Use it for
simple synchronous tools (`hexdump` over TCP, quick integration tests).  For
production servers and clients, use `cb::Event::Server` / `Event::Client`
([WebServer.md](WebServer.md)) which are asynchronous.

Flags passed to `open()` and `accept()`:

```cpp
NONBLOCKING   = 1 << 0   // non-blocking I/O
PEEK          = 1 << 1   // MSG_PEEK on read
NOAUTOCLOSE   = 1 << 2   // don't close fd on destruct
IPV6          = 1 << 3
UDP           = 1 << 4
NOCLOSEONEXEC = 1 << 5
REUSEADDR     = 1 << 6
KEEPALIVE     = 1 << 7
```

Typical client use:

```cpp
Socket::initialize();             // one-time global init (Winsock on Win)
Socket s;
s.open();
s.connect(SockAddr("1.2.3.4:80"));
s.write(reinterpret_cast<const uint8_t*>(req.data()), req.size());
uint8_t buf[4096];
auto n = s.read(buf, sizeof(buf));
```

Typical server use:

```cpp
Socket listener;
listener.open(Socket::REUSEADDR);
listener.bind(SockAddr("0.0.0.0:8080"));
listener.listen();
SockAddr peer;
auto conn = listener.accept(peer, /*flags=*/0);
```

`accept()` returns a `SmartPointer<Socket>` — when the pointer's refcount hits
zero the socket is closed.  Pass `NOAUTOCLOSE` if you'll hand the fd to
something else.

Tunables:

```cpp
s.setReuseAddr(true);
s.setBlocking(false);
s.setCloseOnExec(true);
s.setKeepAlive(true);
s.setSendBuffer(64 * 1024);
s.setReceiveBuffer(64 * 1024);
s.setReceiveTimeout(5.0);      // seconds
s.setSendTimeout(5.0);
s.setTimeout(5.0);             // both
```

Readiness probes (poll-style):

```cpp
if (s.canRead(0.0)) ...        // non-blocking check
if (s.canWrite(2.5)) ...        // wait up to 2.5s
```

`Socket::EndOfStream` is thrown by `read()` when the peer has shut down
cleanly.  Note: this exception inherits from `std::exception` directly, not
`cb::Exception` — a deliberate workaround for a catch-ordering issue
documented in the header.

## SocketSet

Wraps `select(2)`.  Less efficient than libevent, but useful for ad-hoc tools
that need to multiplex a handful of sockets:

```cpp
SocketSet ss;
ss.add(client, SocketSet::READ);
ss.add(listener, SocketSet::READ);
if (ss.select(2.0)) {
  if (ss.isSet(listener, SocketSet::READ)) acceptOne();
  if (ss.isSet(client,   SocketSet::READ)) readOne();
}
```

`select(timeout)` returns true if any socket is ready.  `timeout < 0` blocks
indefinitely, `== 0` polls, `> 0` waits that many seconds.

## URI

`cb::URI` parses RFC 3986 URIs and exposes structured access.  It inherits
from `cb::StringMap`, so the query-string parameters are accessible as map
entries — `uri["page"]` reads the `page=` parameter.

```cpp
URI u("https://user:pw@example.com:8443/a/b?x=1&y=2#frag");

u.getScheme();                     // "https"
u.getHost();                       // "example.com"
u.getPort();                       // 8443
u.getPath();                       // "/a/b"
u.getEscapedPath();                // URL-encoded form
u.getEscapedPathAndQuery();        // "/a/b?x=1&y=2"
u.getPathSegments();               // {"a", "b"}
u.getExtension();                  // ""  (or "html" etc.)
u.getUser();                       // "user"
u.getPass();                       // "pw"
u.getFragment();                   // "frag"

// Query is inherited from StringMap
u["x"];                            // "1"
u.has("y");                        // true
for (auto &[k, v] : u) ...
```

Mutators:

```cpp
URI u;
u.setScheme("https");
u.setHost("api.example.com");
u.setPath("/v1/jobs");
u["page"] = "2";
u["limit"] = "50";
std::string s = u.toString();      // build the URI
```

Constructors accept either a parsed string or piecewise:

```cpp
URI a("https://example.com/x");
URI b("https", "example.com", 443, "/x");
```

Scheme helpers:

```cpp
URI::portFromScheme("https");      // 443
URI::schemeRequiresSSL("wss");     // true
u.schemeRequiresSSL();             // true for https/wss/ftps/...
```

`getPort()` returns the explicit port, or the scheme's default if none was
given.  This is the right call for "what port should I connect to".

Encoding helpers:

```cpp
URI::encode("hello world & friends");        // "hello%20world%20%26%20friends"
URI::decode("hello%20world");                // "hello world"
URI::encode(s, /*unescaped=*/URI::DEFAULT_UNESCAPED);
```

`normalize()` collapses `..`/`.` segments and removes empty path components.
`clear()` resets all fields.

`uri.write(stream)` and `operator<<` produce the canonical string form
without allocating a `std::string`.

## Address ranges and filters

These three classes work together to express "which addresses are allowed
to do X":

  - `AddressRange` — one contiguous range, either CIDR (`10.0.0.0/24`) or
    explicit endpoints (`10.0.0.5-10.0.0.20`)
  - `AddressRangeSet` — collection of ranges with insert/contains/merge
  - `AddressFilter` — allow + deny list combined into a yes/no decision

### AddressRange

```cpp
AddressRange r("10.0.0.0/24");
AddressRange r2("192.168.1.5-192.168.1.10");
AddressRange r3(SockAddr("10.0.0.0"), SockAddr("10.0.0.255"));

r.contains(SockAddr("10.0.0.42"));         // true
r.overlaps(r2);                            // false
r.adjacent(r2);                            // false
r.getCIDRBits();                           // 24 (or -1 if not CIDR-aligned)
r.add(r2);                                 // merge into this range
```

### AddressRangeSet

```cpp
AddressRangeSet allow;
allow.insert("10.0.0.0/8");
allow.insert("192.168.0.0/16");
allow.insert(otherRangeSet);

if (allow.contains(peer)) ...;
LOG_INFO(1, "Whitelist: " << allow);
```

The `insert(spec, dns)` overload accepts a hostname — if a `DNS::Base *` is
passed, it resolves asynchronously and adds the resulting addresses.  Without
a DNS instance, only literal addresses/ranges are accepted.

`AddressRangeSet::write(JSON::Sink&)` serializes to JSON for inclusion in
config dumps.

### AddressFilter

The combination most server code wants:

```cpp
AddressFilter filter(dns);
filter.allow("10.0.0.0/8");
filter.allow("trusted.example.com");
filter.deny("10.0.0.13");

if (!filter.isAllowed(req.getRemoteAddr()))
  THROWX("Forbidden", HTTP_FORBIDDEN);
```

Semantics: allow-list is the default-deny gate; deny-list takes priority.
An empty allow-list permits everything (so the filter degrades to a pure
deny-list when only `deny()` is used).

cbang's HTTP server uses `AddressFilter` to back the standard
`--allow`/`--deny` options ([Config.md](Config.md),
[WebServer.md](WebServer.md)).

## Base64

`cb::Base64` is a parametrized Base64 codec.  Construct with the alphabet
you want, then call `encode()` / `decode()`:

```cpp
Base64 b64;                              // standard alphabet, no wrapping
auto encoded = b64.encode(payload);
auto decoded = b64.decode(encoded);

Base64 wrapped('=', '+', '/', 64);       // wrap at 64 chars per line
URLBase64 url;                            // url-safe: '-' '_', no padding
url.encode(token);
```

`URLBase64` is the standard URL-safe variant used in JWTs and ACMEv2
([ACMEv2.md](ACMEv2.md)).

For binary buffers without going through `std::string`:

```cpp
b64.encode(reinterpret_cast<const char*>(data), len);
```

## Patterns

### Parsing a config-supplied address

```cpp
auto addr = SockAddr::parse(options["bind-address"]);
listener.bind(addr);
```

If the user might supply just an address without a port, use a default:

```cpp
auto s = options["target"];
SockAddr a;
a.read(s);
if (!a.getPort()) a.setPort(443);
```

`read()` returns `false` instead of throwing on parse failure — useful when
you want to fall through to a different format.

### Filtering an HTTP request

```cpp
class GuardedHandler : public Event::HTTPHandler {
  AddressFilter filter;
public:
  GuardedHandler() {
    filter.allow("127.0.0.0/8");
    filter.allow("::1");
  }
  void operator()(Event::Request &req) override {
    if (!filter.isAllowed(req.getClientAddress()))
      THROWX("Forbidden", HTTP_FORBIDDEN);
    // ...
  }
};
```

### Building an API URL

```cpp
URI u("https", server, 0, "/api/v1/jobs");   // 0 → scheme default port
u["status"] = "active";
u["since"]  = since;
client.send(HTTP_GET, u);
```

`URI` handles query-string escaping when you assign through the map
interface; you don't need to call `encode` yourself.

### Building a Unix-socket-aware address parser

```cpp
SockAddr a;
if (s.find('/') != std::string::npos) a = SockAddr(s);          // unix path
else if (!a.read(s)) THROW("Bad address: " << s);
```

This is the pattern used in fah-client to support `unix:/path` as well as
host:port.  (For URI form `unix:` see the URI scheme handling.)

## Portability helpers

  - **`SocketType.h`** defines `socket_t` as `int` on POSIX, `SOCKET` on
    Windows.  Use it instead of either when storing raw fds.
  - **`Winsock.h`** wraps Winsock initialization; `Socket::initialize()`
    calls into it.  Application code rarely includes this directly.
  - **`Swab.h`** provides byte-swap helpers (`bswap16`, `bswap32`,
    `bswap64`) that compile down to single instructions on common
    architectures.  Used by binary protocols (`cbang/pkt/`, fah core
    packet parsing).

## Common pitfalls

  - **`Socket` is blocking.**  In an event-loop program, mixing it with the
    async stack will stall the loop.  Prefer `cb::Event::Client` /
    `Event::Connection`.
  - **`Socket::EndOfStream` is not a `cb::Exception`.**  A bare
    `catch (cb::Exception &)` won't catch it.  Catch it explicitly, or use
    `std::exception` for the catch-all.
  - **`URI::getPort()` falls back to the scheme default.**  Use
    `uri.getPort()` for "what port to connect to"; if you need to know
    whether a port was *specified*, check `uri.toString()` or remember at
    parse time.
  - **`AddressFilter` semantics: empty allow-list permits all.**  This is
    intentional (deny-only deployments), but easy to misread as "deny all".
    Always pair an `allow()` with at least one entry if you want to require
    explicit whitelisting.
  - **`SockAddr::parse` throws.**  Use `read()` if you'd rather get a bool.
  - **`AddressRangeSet::insert` with a hostname needs a `DNS::Base`.**
    Without one, hostnames silently parse-fail.  Pass `dns` explicitly.
  - **`Base64` instances cache encode/decode tables**, so keep one around
    (e.g. as a static or member) rather than constructing on every call.

## See also

  - [WebServer.md](WebServer.md) — async HTTP/HTTPS/WebSocket built on top
  - [EventSystem.md](EventSystem.md) — libevent integration
  - [AsyncDNS.md](AsyncDNS.md) — `DNS::Base` used by `AddressRangeSet` /
    `AddressFilter`
  - [Config.md](Config.md) — `--allow`/`--deny` options consume
    `AddressRangeSet`
  - [ACMEv2.md](ACMEv2.md) — `URLBase64` usage for ACME signatures
