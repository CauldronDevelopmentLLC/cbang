# cbang Event System

cbang's event system is a libevent wrapper that provides timers, file
descriptor watchers, signal handlers, async I/O buffers, and a
thread-pool bridge — all driven from a single event loop.  Every
async API in cbang (HTTP, WebSockets, DNS, subprocesses, ACME) is
built on this foundation.

## Concepts

- **`cb::Event::Base`** — the libevent loop.  One per application is
  the norm; everything else takes it by reference.
- **`cb::Event::Event`** — a single scheduled callback: a one-shot
  timer, a periodic timer, a file-descriptor watch, or a signal
  handler.  Created via the `Base` (which is also an
  `EventFactory`).
- **Flags** — `EVENT_PERSIST`, `EVENT_TIMEOUT`, `EVENT_READ`,
  `EVENT_WRITE`, `EVENT_SIGNAL`, `EVENT_EDGE_TRIG`, `EVENT_FINALIZE`,
  `EVENT_CLOSED`.  Combined as a bitmask.
- **`cb::Event::Buffer`** — an evbuffer wrapper used by sockets and
  HTTP bodies.  Reference-counted; safe to pass by value.
- **`cb::Event::ConcurrentPool`** — runs blocking tasks on worker
  threads and re-enters the loop with the result on the main thread.
- **`cb::Event::EventPtr`** — typedef for `SmartPointer<Event>`.

Everything runs on the loop thread.  Even when work is dispatched to
the concurrent pool, the result callback fires back on the loop
thread, so there is no synchronisation burden in your callbacks.

## The loop

```cpp
#include <cbang/event/Base.h>

cb::Event::Base base(true, 10);   // threads enabled, 10 priority levels

// register handlers / set up clients & servers ...

base.dispatch();                  // blocks; returns when no events remain
                                  // or loopExit() is called
```

| Call | Behavior |
|---|---|
| `dispatch()` | Run forever until no events are pending or `loopExit()`. |
| `loop()` | Single pass; returns when current set drains. |
| `loopOnce()` | Process at least one event, then return. |
| `loopNonBlock()` | Process events that are ready right now; return immediately. |
| `loopBreak()` | Exit the loop now; pending events remain. |
| `loopExit()` | Clean exit when current callback returns. |

`base.getDNS()` and `base.getPool()` return the lazily-created DNS
resolver and FD pool used by the rest of cbang.  See
[AsyncDNS.md](AsyncDNS.md).

## Timers

### One-shot

```cpp
auto event = base.newEvent([] {
  LOG_INFO(1, "fired");
}, 0);                            // 0 = no flags; one-shot
event->add(5);                    // fire 5 seconds from now
```

`newEvent(cb, flags)` returns an `EventPtr`.  Keep it alive — if the
SmartPointer goes out of scope, the event is cancelled.

### Periodic

```cpp
auto event = base.newEvent([] {
  LOG_INFO(1, "tick");
}, cb::EF::EVENT_PERSIST);
event->add(1);                    // fire every 1 second
```

`EVENT_PERSIST` makes the event re-arm automatically with the same
period.  Without it, the event must be re-`add()`ed in the callback.

### Cancel / re-schedule

```cpp
event->del();                     // cancel pending fire
event->add(10);                   // reschedule 10 seconds out
event->activate();                // fire right now
event->isPending();               // bool
```

## File descriptor watchers

```cpp
auto event = base.newEvent(fd, [fd] (cb::Event::Event &, int, unsigned what) {
  if (what & cb::EF::EVENT_READ) { /* readable */ }
  if (what & cb::EF::EVENT_WRITE) { /* writable */ }
}, cb::EF::EVENT_READ | cb::EF::EVENT_PERSIST);
event->add();                     // no timeout; fires on the fd
event->add(30);                   // timeout after 30s if nothing happens
```

The callback signature for fd events is `(Event &, int fd, unsigned
flags)`.  `unsigned flags` indicates which condition fired.

Use `EVENT_EDGE_TRIG` for edge-triggered semantics if your kernel/IO
loop supports it (typically epoll).

## Signal handlers

```cpp
auto sigint = base.newSignal(SIGINT, [this] {
  LOG_INFO(1, "SIGINT received");
  requestExit();
});
sigint->add();                    // arm
```

Like timers and fd events, signal events must be kept alive by their
SmartPointer.  fah-client's `App` ctor stores them as fields:

```cpp
( sigintEvent = base.newSignal(SIGINT,  exitCB))->add();
(sigtermEvent = base.newSignal(SIGTERM, exitCB))->add();
```

## Member-function callbacks

`EventFactory` has overloads that adapt to member functions:

```cpp
class Worker {
public:
  void onTick() { /* ... */ }
  void start() {
    event = base.newEvent(this, &Worker::onTick, cb::EF::EVENT_PERSIST);
    event->add(1);
  }
  cb::Event::EventPtr event;
};
```

The naked `this`-capture form is dangerous if the worker can be
destroyed while the event is pending.  Use `WeakCall`:

```cpp
event = base.newEvent(WeakCall(this, &Worker::onTick), cb::EF::EVENT_PERSIST);
```

See [SmartPointer.md](SmartPointer.md) for the full lifetime
discussion.

## Priorities

The `Base` constructor's second argument is the priority count.  Set
per-event priorities to control firing order when multiple events are
ready in the same iteration:

```cpp
base.initPriority(10);            // 10 priority levels (alternative to ctor)
event->setPriority(6);            // higher number = lower priority
```

fah-client uses this to ensure Unit events run after the
group-scheduler event (`Unit.cpp:117`: `event->setPriority(6)`).

## Buffers

`cb::Event::Buffer` is the evbuffer wrapper used by sockets, HTTP, and
WebSockets.  You'll mostly receive them from those APIs; direct
construction is rare:

```cpp
cb::Event::Buffer buf;
buf.add("hello", 5);
buf.add(otherBuf);
buf.drain(2);                     // consume from the front

// I/O later via Socket or HTTP::Request
```

Buffers are reference-counted via libevent's evbuffer; copying is
cheap.

## Concurrent pool

For blocking or CPU-intensive work that can't run inline on the loop:

```cpp
#include <cbang/event/ConcurrentPool.h>

class HashTask : public cb::Event::ConcurrentPool::Task {
  std::string data, hash;
public:
  HashTask(std::string data) : Task(0), data(std::move(data)) {}
  void run()    override { hash = expensiveHash(data); }   // worker thread
  void success() override { LOG_INFO(1, "hash=" << hash); } // loop thread
  void error(const Exception &e) override { LOG_ERROR(e.getMessage()); }
};

pool.submit(new HashTask("..."));
```

`run()` executes on a pool worker; `success()` / `error()` /
`complete()` are scheduled back to the loop thread when work
finishes.  Use this for filesystem I/O, hashing, parsing large
blobs — anything that would freeze the loop.

### Queued task pattern

`ConcurrentPool::QueuedTask<T>` provides a persistent task that
accepts items via `enqueue()` and processes them serially on the
pool, with results dispatched back on the loop.  Useful for "feed me
work; tell me when each item is done."

## Patterns

### Periodic poller

```cpp
class Poller : public cb::RefCounted {
  cb::Event::EventPtr event;

public:
  Poller(cb::Event::Base &base) {
    event = base.newEvent(WeakCall(this, &Poller::poll),
                          cb::EF::EVENT_PERSIST);
    event->add(5);
  }

  void poll() { /* every 5 seconds */ }
};
```

### Deferred / coalesced work

A common pattern: many incoming events should result in a single
expensive operation.  Use a one-shot event you `activate()`:

```cpp
saveEvent = base.newEvent(WeakCall(this, &App::save), 0);

// ... somewhere, after each change ...
saveEvent->activate();   // calls App::save once at the end of this loop iteration
```

`activate()` is idempotent for non-persistent events: multiple calls
within the same iteration result in one callback.  See
`App::notify` (`App.cpp:550`) and `App::saveGlobalConfig`.

### Graceful shutdown

```cpp
sigint = base.newSignal(SIGINT, [this] {
  LOG_INFO(1, "Shutting down");
  cleanup();
  base.loopExit();
});
sigint->add();
```

`loopExit()` lets the current iteration finish before returning from
`dispatch()`.  `loopBreak()` is the abrupt sibling.

### Spawning child events from inside a callback

Safe — `newEvent` from inside a callback creates a fresh event in
the next iteration:

```cpp
event = base.newEvent([this] {
  // do work ...
  // schedule a follow-up:
  base.newEvent([this] { followUp(); }, 0)->add(0.5);
}, 0);
event->add(0);
```

Keep a strong reference to anything you want to outlive the current
callback.

## Debug logging

```cpp
cb::Event::Event::enableLogging(3);      // verbose libevent log
cb::Event::Event::enableDebugLogging();  // also enable debug mode
```

Helpful when chasing why an event is or isn't firing.  The
`--debug-libevent` option in fah-client wires this up.

## Common pitfalls

- **Letting the `EventPtr` go out of scope.**  The SmartPointer owns
  the event; if you don't store it on the object, it gets cancelled
  immediately.
- **Naked `[this]` lambdas as callbacks.**  Survive past the object's
  destruction → use-after-free.  Use `WeakCall(this, ...)`.
- **Blocking work in a callback.**  Freezes every other timer/IO.
  Use `ConcurrentPool` for blocking calls (filesystem, hashing,
  external processes).
- **Calling `dispatch()` from inside a callback.**  Re-entering the
  loop is undefined.  Use `activate()` / `add()` / `loopExit()`
  to influence the outer dispatch instead.

## See also

- `cbang/event/Base.h`, `cbang/event/Event.h`, `cbang/event/Buffer.h`,
  `cbang/event/ConcurrentPool.h`.
- [SmartPointer.md](SmartPointer.md) — `WeakCall` and event lifetimes.
- [AsyncDNS.md](AsyncDNS.md) — DNS resolver runs on `Base`.
- [WebServer.md](WebServer.md) — HTTP/WS layer on top of events.
- fah-client-bastet `App.cpp` (signals, save event), `Unit.cpp`
  (per-unit event, priority), `Group.cpp` (per-group event,
  `triggerUpdate`), `Account.cpp` (update event + backoff).
