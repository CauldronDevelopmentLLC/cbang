# WebSocket Request and Connection Lifecycle in cbang

## Overview

A WebSocket in cbang involves three distinct objects with different lifetimes:
- **`Connection`** (`HTTP::ConnIn`) — the TCP/TLS socket, owned by `Event::Server`
- **`Request`** — the HTTP upgrade request, owned by `Conn::requests`
- **`Websocket`** — the WebSocket protocol handler, owned by the application

These three objects are deliberately decoupled. `Websocket` holds only a **weak
pointer** to its `Connection` to avoid a reference cycle.

---

## Establishment

1. `Event::Server` accepts a TCP connection and creates a `ConnIn`, storing it
   in `connections`.
2. `ConnIn` reads the HTTP request headers and creates a `Request`, pushing it
   onto `Conn::requests`.
3. The server dispatches the request to a handler (e.g. `Server::websocket()`).
4. The handler constructs a `Websocket` subclass and calls `Websocket::upgrade()`.
5. `upgrade()` performs the HTTP 101 handshake, stores a **weak pointer** to the
   `Connection`, and registers an `onComplete` callback on the `Request`:
   ```cpp
   req.setOnComplete(WeakCall(this, [this] {shutdown();}));
   ```
6. `upgrade()` calls `start()`, which sets `active = true`, calls `onOpen()`,
   begins `readHeader()`, and schedules a ping.

At this point ownership is:
```
Event::Server::connections  →  ConnIn  (strong)
Conn::requests              →  Request (strong)
Application                 →  Websocket (strong, e.g. Account::clients)
Websocket::connection       →  ConnIn  (weak)
```

---

## Normal Operation

- `Websocket` reads frames via `readHeader()` → `readBody()`, each scheduling
  the next read through `Connection::read(WeakCall(this, cb), ...)`.
- The `WeakCall` wrapper ensures that if the `Websocket` is destroyed before a
  pending callback fires, the callback is silently dropped.
- Outbound frames are written via `writeFrame()`, which calls
  `Connection::write(WeakCall(this, cb), ...)` similarly.
- Ping/pong keepalives are managed via `pingEvent` and `pongEvent`.

---

## Shutdown Paths

There are two distinct shutdown paths depending on what initiates the close.

### Path 1: Orderly WebSocket Close

Initiated by either peer sending a `WS_OP_CLOSE` frame, or by the application
calling `Websocket::close()`.

1. `close()` sends a close frame, sets `active = false`, calls `onClose()`, then
   calls `shutdown()`.
2. `shutdown()` promotes the weak `connection` pointer to a strong temporary,
   calls `connection->close()`, then calls `onShutdown()`.
3. `Connection::close()` → `Conn::close()` drains `requests`, calling
   `Request::onComplete()` on each.
4. `Request::onComplete()` fires the callback set during `upgrade()`. Since
   `active` is already `false`, `shutdown()` is a no-op on re-entry.
5. `Conn::close()` calls `Event::Connection::close()` → `FD::close()`, setting
   `fd = -1` and flushing the pool.
6. `Server::remove()` erases the `ConnIn` from `connections`. If no other
   references exist the `ConnIn` is destroyed.

### Path 2: Connection Closed Without WebSocket Handshake

Initiated by a TCP/TLS error, timeout, or the connection being closed from the
network layer without a WebSocket close frame.

1. `Connection::close()` is called directly (e.g. by the FD pool on error).
2. `Conn::close()` drains `requests`, calling `Request::onComplete()`.
3. `Request::onComplete()` fires the callback set during `upgrade()`:
   ```cpp
   WeakCall(this, [this] {shutdown();})
   ```
4. If the `Websocket` is still alive, `shutdown()` is called, setting
   `active = false` and calling `onShutdown()`, which allows the application
   to clean up (e.g. removing from `Account::clients`).
5. `Server::remove()` erases the `ConnIn`. The `Websocket`'s weak pointer
   becomes invalid; `isActive()` returns `false`.

This path ensures `onShutdown()` is always called exactly once regardless of
whether the WebSocket close handshake completed.

---

## `isActive()` vs `active`

```cpp
bool Websocket::isActive() const {
  return active && connection.isSet() && connection->isConnected();
}
```

- `active` — set to `false` by `Websocket::shutdown()`. Reflects whether the
  WebSocket protocol layer has been cleanly shut down.
- `connection->isConnected()` — returns `fd != -1`. Reflects whether the
  underlying TCP connection is still open.

A `Websocket` can be in a state where `active == true` but `isConnected() ==
false` if the connection was closed without going through `Websocket::shutdown()`
first. The `onComplete` callback on the `Request` was introduced specifically to
eliminate this zombie state — any code path that closes the `Connection` will now
reliably trigger `shutdown()`.

---

## Object Destruction Order

When a connection closes cleanly the destruction sequence is:

1. `Request` is popped from `Conn::requests` — its `onComplete` fires `shutdown()`
2. `ConnIn` is erased from `Event::Server::connections` — refcount drops
3. If nothing else holds the `ConnIn` it is destroyed; `Websocket::connection`
   weak pointer becomes null
4. Application releases its `SmartPointer<Websocket>` (e.g. via `onShutdown()`
   removing it from `Account::clients`) — `Websocket` is destroyed

The weak pointer from `Websocket` to `ConnIn` is critical: without it, step 3
would never occur because the `Websocket` would hold the last strong reference
to `ConnIn`, and the `Websocket` would never be destroyed because `ConnIn` holds
`Request` which held a strong `Websocket` reference — a classic reference cycle.
