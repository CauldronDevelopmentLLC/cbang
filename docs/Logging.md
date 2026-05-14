# cbang Logging

cbang ships a leveled, domain-aware logger with built-in rotation,
color, thread/domain prefixes, JSON output, file mirroring, and
listener fan-out.  Use the `LOG_*` macros for normal messages and the
`CATCH_*` / `TRY_CATCH_*` macros for exception handling — these are
the standard idiom across every cbang application.

## Concepts

- **`cb::Logger`** — singleton.  Owns output streams, rotation,
  per-domain levels, listeners.  Configured via `cb::Options` or
  direct setters.
- **Levels** — `RAW`, `ERROR`, `WARNING`, `INFO(n)`, `DEBUG(n)`.
  INFO and DEBUG are subdivided by verbosity (`n`) so callers can
  request "louder" or "quieter" output.
- **Verbosity** — runtime threshold for INFO/DEBUG levels.  Default
  1; raise via `--verbosity=N` or `-v` repeated.
- **Domains** — each log macro is associated with a *domain* (the
  source filename by default).  Per-domain thresholds let you crank
  one subsystem to 5 while leaving everything else at 1.
- **Prefix** — per-thread string prepended to every line.  Used for
  contextual tagging (e.g. fah-client's `WU1:`, `Default:`).
- **Listeners** — `cb::LogListener` / `cb::LogLineListener`
  subclasses that receive each emitted line.  Used for live tail to
  a UI, structured forwarding, etc.

## The macros

```cpp
#include <cbang/log/Logger.h>

LOG_ERROR  ("Bad cert: "  << err);              // always logged
LOG_WARNING("Retry "      << n);                // always logged
LOG_INFO (1, "Started");                        // level-gated info
LOG_INFO (3, "User: "     << user);
LOG_DEBUG(5, "raw bytes: " << buf.toHexString());
LOG_RAW  ("plain text\n");                      // bypass all formatting
```

| Macro | Level | Verbosity arg |
|---|---|---|
| `LOG_RAW(msg)` | unformatted bypass | — |
| `LOG_ERROR(msg)` | always shown | — |
| `LOG_WARNING(msg)` | always shown | — |
| `LOG_INFO(n, msg)` | INFO @ level n | yes |
| `LOG_DEBUG(n, msg)` | DEBUG @ level n | yes |

Messages are C++ `iostream` expressions; anything with `operator<<`
works.  Concatenate as you would with `cout`:

```cpp
LOG_INFO(1, "User " << user << " has " << count << " items");
```

In a release build, `LOG_DEBUG` is compiled out entirely (no
formatting cost) when `DEBUG` is not defined.

## Exception macros

Combined idiom: emit a log line for any exception thrown in a code
block.

```cpp
TRY_CATCH_ERROR(parseAndApply(input));
TRY_CATCH_WARNING(notifyRemote(msg));
TRY_CATCH_DEBUG(3, slowCheck());
```

Or split:

```cpp
try {
  parseAndApply(input);
} CATCH_ERROR;

try {
  parseAndApply(input);
} CATCH_INFO(3);
```

The exception's message and stack location are included in the
output.  Use these instead of bare `try`/`catch` so failures get a
consistent, locatable trail.

## Output destinations

By default the logger writes to stdout (with color if attached to a
TTY) and to a log file (`log.txt` by default).  Both are
configurable:

```cpp
Logger::instance().setLogFile("/var/log/myapp.log");
Logger::instance().setLogToScreen(false);
Logger::instance().setLogColor(false);
```

### Listeners

Attach a listener to receive every line:

```cpp
class MyListener : public cb::LogLineListener {
public:
  void logUpdate(const cb::JSON::ValuePtr &lines, uint64_t last) override {
    // lines is a JSON list of strings; forward / index / pipe to UI
  }
};

Logger::instance().addListener(new MyListener);
```

fah-client uses this to stream live log lines to the web frontend
via `LogTracker` (`LogTracker.cpp`).

### Tail a file into the logger

`cb::TailFileToLog` watches a file and emits each new line through
the logger with a chosen prefix.  Used by fah-client's `Unit` to
copy a science core's `logfile_01.txt` into the main log
(`Unit.cpp:1278`):

```cpp
logCopier = new cb::TailFileToLog(base, "core/logfile.txt", "WU1:");
```

## Verbosity and per-domain levels

The threshold above which a level is silent:

```cpp
Logger::instance().setVerbosity(3);
```

Per-domain thresholds via the `--log-domain-levels` option (or by
direct API).  Format: `domain:level domain:i:level domain:d:level
domain:t:level`:

| Suffix | Meaning |
|---|---|
| (none) | Set domain's overall threshold |
| `:i` | INFO-only threshold |
| `:d` | DEBUG-only threshold |
| `:t` | Enable traces in this domain |

Example:
```
--log-domain-levels="net:5 db:i:3 fah/client/Unit:d:6"
```
Says: net domain at 5 (verbose), db at INFO level 3, Unit at DEBUG
level 6.

The default domain is the source `__FILE__` of the macro
invocation.  Override with `#define CBANG_LOG_DOMAIN "myname"`
inside a file or block.

## Prefixes

Per-thread prefix prepended to every line emitted from that thread:

```cpp
Logger::instance().setPrefix("worker-1:");
LOG_INFO(1, "starting");           // → "worker-1: starting"
```

### Scoped prefixes

`cb::SmartLogPrefix` is RAII: pushes a prefix on construction,
restores the old one on destruction.

```cpp
{
  cb::SmartLogPrefix p("WU" + std::to_string(wuID) + ":", /*append*/ true);
  LOG_INFO(1, "Downloading");
  LOG_INFO(1, "Done");
}
// prefix restored here
```

With `append=true` the new prefix is concatenated to the existing
one; without it the prefix is replaced.

### Per-class prefix via `CBANG_LOG_PREFIX`

Override the macro inside an `.cpp` file to add a class-specific
prefix to every log line in that translation unit:

```cpp
#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX getLogPrefix()
```

`getLogPrefix()` is a member function returning a string — this is
the pattern fah-client's `Unit` uses (`Unit.cpp:111-112`) to tag
every line with `WU<N>:`.

## Configuration via `cb::Options`

If your app uses cbang's options system, `Logger::addOptions()`
registers the standard logging options.  These appear in `--help`
under "Logging":

| Option | Default | Meaning |
|---|---|---|
| `verbosity` | 1 | INFO/DEBUG verbosity threshold |
| `log` | `log.txt` | Log file path |
| `log-color` | true | ANSI color on screen output |
| `log-to-screen` | true | Mirror to stdout |
| `log-truncate` | false | Truncate log file on each run |
| `log-rotate` | true | Rotate log on startup |
| `log-rotate-dir` | `logs` | Directory for rotated logs |
| `log-rotate-max` | 0 | Keep at most N rotated files |
| `log-rotate-period` | 0 | Re-rotate every N seconds (0 = startup only) |
| `log-rotate-compression` | NONE | `NONE`, `GZIP`, `BZIP2`, `ZSTD`, `XZ` |
| `log-time` | true | Print time on each line |
| `log-date` | false | Print date on each line |
| `log-date-periodically` | 0 | Date heading every N seconds |
| `log-level` | true | Print `I`/`W`/`E` level marker |
| `log-short-level` | false | Print single-char level |
| `log-domain` | false | Print domain on each line |
| `log-simple-domains` | true | Strip path/extension from domain names |
| `log-thread-id` | false | Print thread id |
| `log-thread-prefix` | true | Print per-thread prefix |
| `log-header` | true | Header on each entry |
| `log-no-info-header` | false | Skip header for INFO-level lines |
| `log-crlf` | false | Use CRLF line endings |
| `log-debug` | (DEBUG build) | Enable DEBUG output at all |
| `debug-libevent` | false | Enable libevent's own debug log |
| `log-domain-levels` | "" | Per-domain levels (see above) |

`cb::Logger::instance().initEvents(base)` integrates rotation and
periodic dates with the event loop.  Call it once after constructing
the `cb::Event::Base`.

## Patterns

### Per-WU prefix

```cpp
class Unit {
  std::string getLogPrefix() const {
    return cb::String::printf("WU%" PRIu64 ":", wu);
  }
};

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX getLogPrefix()
```

Every `LOG_*` in this TU now tags lines with `WU42:` etc.

### Suppress logging in test mode

```cpp
Logger::instance().setLogToScreen(false);
Logger::instance().setLogFile("/dev/null");
```

Useful for unit test runners that diff stdout.

### Stream a subprocess's output

```cpp
auto copier = cb::SmartPtr(new cb::TailFileToLog(base, "stdout.log", "child:"));
// destruct to stop watching
```

### Conditional debug

```cpp
if (CBANG_LOG_DEBUG_ENABLED(5))
  LOG_DEBUG(5, "expensive dump: " << buildLargeString());
```

The macro short-circuits when verbosity won't show the line, saving
the cost of building the message.

### Forward log to a WebSocket / SSE consumer

```cpp
class WSLogListener : public cb::LogLineListener {
  cb::WS::JSONWebsocket &ws;
public:
  WSLogListener(cb::WS::JSONWebsocket &ws) : ws(ws) {}
  void logUpdate(const cb::JSON::ValuePtr &lines, uint64_t last) override {
    ws.send(*lines);
  }
};

Logger::instance().addListener(new WSLogListener(ws));
```

This is how the frontend's live log view is fed in fah-client.

## Common pitfalls

- **Forgetting `initEvents(base)`.**  Rotation timers and periodic
  date headers won't fire without it.
- **Logging in destructors during shutdown.**  Logger may already be
  finalising.  Prefer guarded `TRY_CATCH_ERROR` and keep destructors
  quiet.
- **Streaming huge buffers at INFO 1.**  Each `<<` is evaluated even
  when the level is above threshold; the macro decides whether to
  *emit*, not whether to *build*.  Use `LOG_DEBUG_ENABLED` /
  `LOG_INFO_ENABLED` to gate expensive formatting.
- **Confusing level vs verbosity.**  ERROR/WARNING are always shown;
  INFO(n)/DEBUG(n) are gated by verbosity.  `LOG_INFO(5, ...)`
  appears only if verbosity ≥ 5.

## See also

- `cbang/log/Logger.h`, `cbang/log/SmartLogPrefix.h`,
  `cbang/log/TailFileToLog.h`, `cbang/log/LogLineListener.h`.
- `cbang/Catch.h` for the `CATCH_*` / `TRY_CATCH_*` macros.
- [Config.md](Config.md) — `cb::Options` provides the logging
  configuration knobs.
- [EventSystem.md](EventSystem.md) — `Logger::initEvents` wires
  rotation/date timers to the loop.
- fah-client-bastet `Unit.cpp` (per-WU prefix), `App.cpp` (logger
  defaults + listener), `LogTracker.cpp` (frontend fan-out),
  `Unit.cpp::startLogCopy` (`TailFileToLog`).
