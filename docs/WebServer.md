# cbang Web Server / Client

cbang provides an event-driven HTTP/HTTPS/WebSocket stack built on
libevent.  A single event loop runs every I/O operation: clients and
servers share an `Event::Base`, callbacks fire from the loop thread,
and nothing blocks.  Most non-trivial cbang applications use this
layer; the Folding@home client (`fah-client-bastet`) is the canonical
example and is referenced throughout this document.

## Concepts

- **`cb::Event::Base`** — the libevent loop owned by your application.
  Construct one; everything else takes it by reference.
- **`cb::HTTP::Client`** — outgoing requests.  One per logical
  connection pool; usually one per application.
- **`cb::HTTP::Server`** — incoming requests.  Subclass it and register
  handlers.
- **`cb::HTTP::Request`** — a single in-flight request/response, used
  on both sides.  Passed to your callbacks.
- **`cb::WS::Websocket` / `cb::WS::JSONWebsocket`** — WebSocket
  endpoints.  Subclass and override `onMessage`, `onOpen`, `onClose`.
- **`cb::SSLContext`** — TLS configuration.  Pass to clients and
  servers; required for `https`/`wss`.

The event loop is single-threaded.  Lifetimes are managed by
`cb::SmartPointer` (refcounted).  See [Lifetime and callbacks](#lifetime-and-callbacks) below.

## Setup

```cpp
#include <cbang/event/Base.h>
#include <cbang/http/Client.h>
#include <cbang/openssl/SSLContext.h>

using namespace cb;

Event::Base   base(true, 10);              // threads enabled, 10 priorities
HTTP::Client  client(base, new SSLContext); // shared SSL ctx; null = HTTP only

client.setReadTimeout(300);
client.setWriteTimeout(300);
```

Run the loop with `base.dispatch()` once your handlers are registered
and your outgoing requests are queued.  Quit with `base.loopExit()`.

If you derive from `cb::Application`, the base and client are usually
fields of your App class.  See `App.cpp` in fah-client-bastet for the
pattern.

## HTTP client

### Issue a request

`Client::call(uri, method, callback)` constructs a `PendingRequest`,
which you finish setting up (body, headers) and then `send()`.  The
callback fires when the response arrives or the connection fails.

```cpp
URI uri("https://example.com/api/thing");

auto cb = [] (HTTP::Request &req) {
  if (req.getConnectionError()) { /* network failure */ return; }
  if (!req.isOk())              { /* HTTP error: req.getResponseCode() */ return; }

  std::string body = req.getInput();          // raw body
  auto json        = req.getInputJSON();      // parsed JSON, or null
};

auto pr = client.call(uri, HTTP_GET, cb);
pr->send();
```

`PendingRequest::send()` queues the request on the connection.  The
returned `RequestPtr` keeps the request alive — store it until the
callback fires or you want to cancel via `pr.release()`.

### Member-function callbacks

`Client::call` has overloads that take `(this, &Class::method)` and
adapt automatically:

```cpp
class Worker {
  void onResponse(HTTP::Request &req) { ... }
  void start() {
    pr = client.call(uri, HTTP_GET, this, &Worker::onResponse);
    pr->send();
  }
  HTTP::Client::RequestPtr pr;
};
```

This is the form fah-client uses everywhere — see
`Unit::assign` for the POST pattern (`Unit.cpp:1059`) and `Core::download` for the GET
pattern (`Core.cpp`).

### POST a JSON body

`Request::send(callback)` accepts a function that writes into a
`JSON::Sink`:

```cpp
pr = client.call(uri, HTTP_POST, this, &Worker::onResponse);
pr->getRequest()->send([&] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("user", "alice");
  sink.insert("count", 42);
  sink.endDict();
});
pr->send();
```

For raw bytes use `send(buf)`, `send(data, length)`, or `send(string)`.

### Progress callbacks

The connection exposes read/write progress.  fah-client uses this for
download progress on WU data and core binaries:

```cpp
auto onProgress = [this] (const Progress &p) {
  // p.getTotal(), p.getSize()
};
pr = client.call(uri, HTTP_GET, this, &Self::onResponse);
pr->getConnection()->getReadProgress().setCallback(onProgress, 1);
pr->send();
```

### Cancelling and timeouts

`pr.release()` cancels.  Per-client timeouts are set with
`setReadTimeout` / `setWriteTimeout` (seconds).  For per-request
timeouts use the request's own setters.

### Response inspection

| Call | Returns |
|---|---|
| `req.isOk()` | true iff status is `HTTP_OK` |
| `req.getResponseCode()` | status enum |
| `req.getInput()` | response body as `string` |
| `req.getInputJSON()` | parsed `JSON::ValuePtr` (null on parse error) |
| `req.getJSONMessage()` | as above, but cached |
| `req.getInputStream()` | streaming reader |
| `req.getConnectionError()` | true on transport-level failure |
| `req.logResponseErrors()` | logs and returns true if status not OK |

## HTTP server

### Subclass `HTTP::Server`

```cpp
class MyServer : public HTTP::Server {
  App &app;
public:
  MyServer(App &app) : HTTP::Server(app.getBase(), app.getSSLCtx()), app(app) {}

  void init() {
    auto &opts = app.getOptions();

    // CORS preflight / origin check
    addMember(this, &MyServer::corsCB);

    // Specific routes
    addMember(HTTP_GET, "/ping",          this, &MyServer::ping);
    addMember(HTTP_GET, "/api/websocket", this, &MyServer::handleWebsocket);

    // Static files
    if (opts["web-root"].hasValue())
      addHandler("/.*", opts["web-root"].toString());

    // Call base init AFTER your routes — it binds listen ports.
    HTTP::Server::init(opts);

    // Catch-all redirect (last)
    addMember(HTTP_GET, "/.*", this, &MyServer::redirect);
  }

  bool ping(HTTP::Request &req) {
    req.reply(HTTP_OK, "pong");
    return true;
  }

  bool handleWebsocket(HTTP::Request &req);   // see WebSocket section
};
```

Handler matching is in registration order.  A handler returns `true`
to claim the request, `false` to fall through.

See `Server.cpp` in fah-client-bastet for the production version.

### Listening

`HTTP::Server::init(options)` reads `http-addresses` (and the
corresponding TLS option) and calls `addListenPort` /
`addSecureListenPort` for each entry.  You can also call those
directly with a `SockAddr` of your own.

### Handler signatures

Three forms registered via `addMember` / `addHandler`:

```cpp
// Method + pattern + member
addMember(HTTP_GET, "/path/[a-z]+", this, &Self::handler);

// Pattern + member (any method)
addMember("/api/.*", this, &Self::handler);

// Catch-all member (any method, any pattern)
addMember(this, &Self::handler);
```

Patterns are RE2 regex.  Add static files with
`addHandler(pattern, fsPath)` or `addHandler(pattern, resource)`.

### Replying

```cpp
req.setContentType("application/json");
req.reply(HTTP_OK, "{\"ok\":true}");
```

For streamed JSON:

```cpp
req.reply(HTTP_OK, [] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("ok", true);
  sink.endDict();
});
```

For larger payloads, get an output stream:

```cpp
auto stream = req.getOutputStream(COMPRESSION_GZIP);
*stream << "long body...";
req.reply(HTTP_OK);
```

`reply()` is one-shot; calling it twice is a bug.  Use `send()` calls
to *append* to the output before the final `reply()`.

### TLS

Pass an `SSLContext` to the server constructor and the secure listen
port serves HTTPS.  Same context can be shared with the client.

```cpp
SmartPointer<SSLContext> ctx = new SSLContext;
ctx->loadSystemRootCerts();
ctx->useCertificateChainFile("server.pem");
ctx->usePrivateKey("server.key");

HTTP::Client client(base, ctx);
MyServer    server(app);            // takes ctx in its ctor
```

## WebSocket server

A WebSocket on the server side is a regular HTTP handler that
*upgrades* the connection.  Subclass `WS::Websocket` or
`WS::JSONWebsocket` (handles framing JSON for you):

```cpp
class MyWS : public WS::JSONWebsocket {
public:
  void onOpen() override { /* welcome msg */ }
  void onMessage(const JSON::ValuePtr &msg) override { /* handle */ }
  void onClose(WS::Status status, const std::string &msg) override { /* cleanup */ }
};

bool MyServer::handleWebsocket(HTTP::Request &req) {
  auto ws = SmartPtr(new MyWS);
  ws->upgrade(req);            // performs handshake; takes ownership of the conn
  app.add(ws);                 // keep a reference somewhere so it stays alive
  return true;
}
```

`upgrade(req)` validates the WebSocket handshake (checks
`Upgrade: websocket` and `Sec-WebSocket-Key`), sends the response, and
binds this WS object to the connection.

Lifetime: the WS object must outlive the connection; keep a strong
reference (fah-client's `App::add(remote)` is the equivalent).

Send a message:

```cpp
ws.send(jsonValue);
```

`JSONWebsocket` serializes the value and frames it.  Use plain
`Websocket::send(string)` for raw frames.

See `WebsocketRemote.h/cpp` in fah-client-bastet for the production
shape.

## WebSocket client

Same `Websocket`/`JSONWebsocket` classes — call `connect()` instead of
`upgrade()`:

```cpp
class AccountWS : public WS::JSONWebsocket {
public:
  AccountWS(HTTP::Client &c, const URI &uri) {
    connect(c, uri);                 // wss://... or ws://...
  }

  void onOpen() override                                { /* login msg */ }
  void onMessage(const JSON::ValuePtr &msg) override    { /* handle   */ }
  void onClose(WS::Status status, const std::string &m) { /* retry?   */ }
};
```

The client manages pings/pongs and reconnection logic is up to you.
fah-client's `Account` class (`Account.cpp`) implements a state machine
(IDLE → LINK → INFO → CONNECT → CONNECTED) with exponential backoff via
`cb::Backoff`.

## Lifetime and callbacks

The single biggest source of bugs in async cbang code is dangling
references: the loop fires a callback after the object that owns it
has gone away.

**Strong refs.**  `SmartPointer<T>` is the default — refcounted, holds
the target alive.  Use for objects you own.

**`PhonyPtr(this)`.**  A `SmartPointer` wrapper around a raw pointer
that does NOT manage lifetime.  Used to bind a lambda or callback to
`this` without bumping the refcount.

**`WeakCall(this, &Self::method)`.**  Wraps a member-function callback
so that if `this` has been destroyed, the callback no-ops instead of
crashing.  Use this when registering long-lived event callbacks on
`this` that you do *not* want to extend the object's lifetime:

```cpp
event = base.newEvent(WeakCall(this, &Self::tick), 0);
event->add(60);
```

If you instead use a plain lambda `[this] { tick(); }`, the lambda
will fire even after `this` is destroyed and crash.

**Callback ownership of `PendingRequest`.**  Store the
`RequestPtr` somewhere; if it goes out of scope the request is
cancelled.  fah-client stores it as `pr` on the Unit
(`Unit.h: cb::HTTP::Client::RequestPtr pr`).

## Options

Both `HTTP::Client` and `HTTP::Server` accept options via
`cb::Options`.  If you derive from `cb::Application`, the standard set
appears in `--help`:

| Option | Used by | Effect |
|---|---|---|
| `http-addresses` | server | Listen addresses (`host:port` list).  Default `127.0.0.1:80`. |
| `https-addresses` | server | TLS listen addresses. |
| `connection-timeout` | server | Per-connection idle timeout. |
| `connection-backlog` | server | listen() backlog. |
| `max-connections` | server | Per-port cap. |
| `allow` / `deny` | server | IP filter rules. |
| `http-max-body-size` | server | Reject oversized bodies. |
| `http-max-headers-size` | server | Reject oversized headers. |

Server options are registered when you call `addOptions(options)`
during App construction (cbang's `Application` does this for you).
Server options are read inside `Server::init(options)`.

## URIs

`cb::URI` parses RFC 2396 URIs.  Common patterns:

```cpp
URI uri("https://example.com/api/x?k=v");
uri.getScheme();   // "https"
uri.getHost();
uri.getPort();     // 0 if not set; HTTP::Client uses portFromScheme
uri.getPath();
uri.getQuery();
```

Construct programmatically:

```cpp
URI uri("https", "example.com", 0, "/api/assign");
```

WebSocket URIs use `ws://` / `wss://` schemes.

## End-to-end: HTTP POST with JSON, signed body, JSON reply

This is the pattern used for fah-client's assignment server requests.
The full implementation is in `Unit::assign` (`Unit.cpp:1059`); the
sketch:

```cpp
URI uri("https", "assign.example.com", 0, "/api/assign");

pr = client.call(uri, HTTP_POST, this, &Self::onResponse);

pr->getRequest()->send([&] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("data", *requestData);
  sink.insert("signature", Base64().encode(signature));
  sink.insert("pub-key",   key.publicToPEMString());
  sink.endDict();
});

pr->send();
```

And the callback:

```cpp
void Self::onResponse(HTTP::Request &req) {
  pr.release();

  if (req.getConnectionError()) { /* retry */ return; }

  if (req.logResponseErrors()) {
    switch (req.getResponseCode()) {
    case HTTP_SERVICE_UNAVAILABLE: /* retry later */; break;
    case HTTP_BAD_REQUEST:         /* give up   */; break;
    }
    return;
  }

  auto msg = req.getInputJSON();
  // ...process msg...
}
```

## See also

- `Client.h`, `Server.h`, `Request.h` for the full APIs.
- `Websocket.h`, `JSONWebsocket.h` for the WS classes.
- `Event::Base` for the loop, `Event::Event` for timers/signals.
- fah-client-bastet for production examples — particularly
  `App.cpp`, `Server.cpp`, `WebsocketRemote.cpp`, `Account.cpp`,
  `Unit.cpp` (`assign`, `download`, `upload`, `dump`).
