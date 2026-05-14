# cbang Exceptions and Error Handling

This document covers cbang's exception system: `cb::Exception`, the predefined
error subclasses, the `THROW`/`CATCH`/`ASSERT` macros, file-location and
stack-trace integration, and patterns for use in application code.

## Overview

cbang centralizes error reporting on a single rich exception type
(`cb::Exception`) and a family of macros that capture source location, optional
cause chains, optional numeric codes, and (in debug builds) stack traces.
Compared to throwing raw `std::runtime_error`, this gives you:

  - A consistent printable form (`what()`, `print()`, `toString()`)
  - File/line/column of the throw site, automatically
  - Nested cause chains so you can rethrow with added context
  - Numeric codes (used by the API framework to select HTTP status)
  - Optional stack traces for postmortem debugging
  - A standardized subclass hierarchy (`KeyError`, `IOError`, etc.)

The same macros work whether the application defines `USING_CBANG` (which
exposes short `THROW`, `CATCH`, etc.) or only `CBANG_THROW` / `CBANG_CATCH` to
avoid collisions with other libraries.

## Headers

```cpp
#include <cbang/Exception.h>     // class cb::Exception
#include <cbang/Throw.h>         // THROW, THROWC, THROWX, ASSERT
#include <cbang/Catch.h>         // CATCH, TRY_CATCH and variants
#include <cbang/Errors.h>        // KeyError, IOError, ParseError, ...
```

In most application code, `#include <cbang/Catch.h>` and `#include
<cbang/Errors.h>` are enough; they pull in everything else.

To get the short macros (`THROW`, `CATCH`, …) instead of the `CBANG_`-prefixed
forms, define `USING_CBANG` in the project's build configuration (cbang-based
projects almost always do).

## Throwing exceptions

### Basic throws

```cpp
THROW("Unexpected value: " << value);
```

`THROW(msg)` constructs a `cb::Exception` with the streamed message and the
current source location (file, line, column captured by `__FILE__`,
`__LINE__`).  `msg` is an arbitrary `operator<<` chain — anything that works
with `std::ostream` works here.

### Throw with a cause

When catching one exception and rethrowing with additional context:

```cpp
try { parseConfig(filename); }
catch (const cb::Exception &e) { THROWC("Loading config failed", e); }
```

`THROWC(msg, cause)` chains the caught exception as the cause of the new one.
`Exception::getMessages()` walks the chain and joins messages with `: `, and
`Exception::print()` prints each cause indented below its parent.

### Throw with a code

```cpp
THROWX("Not found", HTTP_NOT_FOUND);
```

`THROWX(msg, code)` sets `Exception::code`.  The API framework maps these to
HTTP status codes (see [API.md](API.md)).  `Exception::getTopCode()` walks the
cause chain looking for the first nonzero code.

### Throw with both

```cpp
THROWCX("Auth failed", cause, HTTP_UNAUTHORIZED);
```

### Throw a specific subclass

`Errors.h` provides predefined subclasses and shorthand macros:

```cpp
KEY_ERROR("No such field: " << name);   // throws cb::KeyError
TYPE_ERROR("Expected string");          // throws cb::TypeError
SYSTEM_ERROR("open() failed: " << SysError::getMessage());
IO_ERROR("Short write");
PARSE_ERROR("Unexpected token at line " << lineno);
REFERENCE_ERROR("Use after free");
NOT_IMPLEMENTED_ERROR();                // standard "Not implemented" msg
CAST_ERROR();                           // standard "Invalid Cast" msg
```

For your own subclasses use:

```cpp
namespace mything {
  CBANG_DEFINE_EXCEPTION_SUBCLASS(ConfigError);          // : cb::Exception
  CBANG_DEFINE_EXCEPTION_SUPER(WrappedError, OtherBase); // : OtherBase
}

THROWT(mything::ConfigError, "Missing key: " << k);
```

`CBANG_DEFINE_EXCEPTION_SUBCLASS` generates a struct that inherits all of
`cb::Exception`'s constructors via `using`, so the subclass behaves identically
except for its type identity.  `THROWT`, `THROWTC`, `THROWTX`, `THROWTCX`
throw a specific type instead of `CBANG_EXCEPTION`.

### Asserts

```cpp
ASSERT(ptr != 0, "Null pointer in " << __func__);
```

`ASSERT` throws if the condition is false, in **debug builds only**.  In
release builds it compiles to nothing — do not use it for input validation or
anything whose side effects you rely on.  Use `THROW` directly when the check
must always run.

## Catching exceptions

### The CATCH macros

Plain `try { ... } catch (...) { ... }` works, but `cbang/Catch.h` provides
macros that log the exception with file/line attribution at a chosen log
level.  This is the idiomatic form in cbang-based code:

```cpp
try {
  doSomething();
} CATCH_ERROR;
```

`CATCH_ERROR` expands to three catch blocks (`cb::Exception`, `std::exception`,
`...`) that log to the error level via `LOG_ERROR`.  In release builds
non-cbang exceptions are also caught and logged; in debug they propagate so a
debugger can inspect them.

Variants:

  - `CATCH_ERROR`            — log at error level
  - `CATCH_WARNING`          — log at warning level
  - `CATCH_INFO(level)`      — log at the given info sub-level
  - `CATCH_DEBUG(level)`     — log at debug level (suppressed in release)
  - `CATCH(level, msg)`      — fully custom; `msg` is streamed into the log
    line, and the variable `msg` (string) is available inside the catch body
    if you need the exception text

The "one-shot" form wraps the try as well:

```cpp
TRY_CATCH_ERROR(unit->save());
TRY_CATCH_WARNING(notifyClients());
TRY_CATCH_DEBUG(3, expensiveTrace());
```

These are the standard pattern for fire-and-forget calls that may throw but
must not propagate (event callbacks, destructors, background tasks).

### Inspecting an exception

Inside a catch block, you typically use `e.what()`, but the full API is:

```cpp
} catch (const cb::Exception &e) {
  LOG_ERROR("Failed: " << e);              // operator<< calls print()
  std::string s = e.toString();            // single-line form
  std::string chain = e.getMessages();     // "msg1: msg2: msg3" via causes
  int code = e.getTopCode();               // first nonzero code in chain
  auto cause = e.getCause();               // SmartPointer<Exception> or null
  auto trace = e.getStackTrace();          // may be null
}
```

For programmatic use (JSON APIs):

```cpp
JSON::Builder builder;
e.write(builder, /*withDebugInfo=*/false);  // serializes the exception
```

`withDebugInfo=false` strips file/line/stack so internal locations don't leak
to remote clients.

## File location and stack traces

`Exception::FileLocation` captures `__FILE__`, `__LINE__`, and `__COLUMN__` (if
available) at the throw site, via the `CBANG_FILE_LOCATION` macro that all the
`THROW*` macros use.  Printing an exception includes the location by default;
this can be globally disabled:

```cpp
cb::Exception::printLocations = false;
```

Stack traces are off by default (capturing them is expensive).  Enable them at
process start when you want them on every throw:

```cpp
cb::Exception::enableStackTraces = true;
```

When enabled, `cb::Exception`'s constructor captures the current stack via
`cbang/debug/StackTrace.h` and stores it in the exception.  The cbang
`Application` base class typically wires this up under `--debug` or
`--stack-traces`; see [Application.md](Application.md).

`Exception::causePrintLevel` caps how deep `print()` will recurse into the
cause chain — useful when an exception is rethrown many times.

## Code-to-HTTP-status mapping

The HTTP API framework uses `Exception::getTopCode()` to choose the response
status:

```cpp
void MyHandler::operator()(Event::Request &req) {
  auto group = groups->get(req.getArg("group"));
  if (group.isNull()) THROWX("Unknown group", HTTP_NOT_FOUND);
  // ...
}
```

A throw deep inside `groups->get()` that sets `code = HTTP_NOT_FOUND` will
propagate up and `HTTP::Server` will return a 404 with the exception's
message as the response body.  See [API.md](API.md) and
[WebServer.md](WebServer.md).

Codes are just `int`, so you can use them for application-specific error codes
too — JSON-RPC error codes, internal classifications, etc.  Anything that
isn't a recognized HTTP status falls back to 500.

## Patterns

### Adding context as you unwind

The cause-chain pattern keeps each layer focused on its own concern:

```cpp
void loadCert(const string &path) {
  try { cert.readPEM(path); }
  catch (const cb::Exception &e) { THROWC("Reading cert " + path, e); }
}

void initSSL() {
  try { loadCert("/etc/foo.pem"); loadCert("/etc/bar.pem"); }
  catch (const cb::Exception &e) { THROWC("SSL init", e); }
}
```

A failure on `bar.pem` produces:

```
SSL init: Reading cert /etc/bar.pem: ENOENT: No such file
```

with file/line for each layer when stack traces are off and a full trace when
they're on.

### Translating system errors

```cpp
int fd = ::open(path.c_str(), O_RDONLY);
if (fd < 0) THROW("open(" << path << "): " << SysError::getMessage());
```

`cbang/os/SysError.h` formats `errno` (or `GetLastError()` on Windows) into a
string.  The shorthand `SYSTEM_ERROR(msg)` (`Errors.h`) throws a
`cb::SystemError` with the same idiom.

### Don't let exceptions escape async callbacks

libevent callbacks must not throw across the C event loop.  Wrap with
`TRY_CATCH_ERROR`:

```cpp
event->add([this] {
  TRY_CATCH_ERROR(this->onTimer());
});
```

The same applies to destructors and signal handlers.  See
[EventSystem.md](EventSystem.md).

### Don't catch what you can't handle

Resist the urge to wrap every call in a `try`.  Let exceptions propagate to a
layer that has the context to do something useful — usually the request
handler, the event-loop callback, or `main()`.  The cbang `Application` base
class already installs a top-level catch around `run()`.

## Common pitfalls

  - **`ASSERT` is compiled out in release.**  If the check is required for
    correctness, use `THROW` (or `if (!cond) THROW(...);`).
  - **Throwing during stack unwinding.**  If a destructor throws while another
    exception is in flight, `std::terminate` is called.  Always wrap
    destructor cleanup in `TRY_CATCH_ERROR`.
  - **Slicing.**  Catching `cb::Exception` by value loses the dynamic type
    (`KeyError`, `IOError`, …).  Catch by `const cb::Exception &`.
  - **`getMessage()` vs `getMessages()`.**  The first returns only this
    exception's message; the second walks the cause chain.  `what()` returns
    just the local message and matches `std::exception`.
  - **`getCode()` vs `getTopCode()`.**  The first is local; the second walks
    the cause chain to find the first nonzero code.  HTTP status mapping uses
    `getTopCode()`.
  - **Leaking internal detail over HTTP.**  Use
    `Exception::write(sink, /*withDebugInfo=*/false)` (or arrange for the API
    layer to do so) when serializing exceptions to remote clients.

## See also

  - [Application.md](Application.md) — top-level catch in `App::run()`,
    `--stack-traces` option
  - [API.md](API.md) — exception → HTTP status mapping
  - [WebServer.md](WebServer.md) — request handlers and error responses
  - [Logging.md](Logging.md) — `LOG_*` macros used by `CATCH_*`
  - [EventSystem.md](EventSystem.md) — async callbacks and `TRY_CATCH_ERROR`
