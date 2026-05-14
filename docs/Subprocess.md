# cbang Subprocess and OS Utilities

cbang's `os/` and `event/` namespaces expose portable wrappers for
the operating system: spawning child processes (sync and async),
filesystem operations, glob matching, single-instance locks, idle
and battery detection, process priority, system info, and exit
signals.  These are the daily-driver utilities — used by almost
every cbang application.

## Concepts

- **`cb::Subprocess`** — synchronous child process: spawn, wait,
  read exit code.  Captures stdout/stderr to pipes.
- **`cb::Event::AsyncSubprocess` / `Event::SubprocessPool`** —
  event-driven variants that integrate with `Event::Base`.
- **`cb::SystemUtilities`** — namespace function library:
  `mkdir`, `exists`, `read`, `write`, `unlink`, `rotate`, path
  manipulation, etc.  Free functions, not a class.
- **`cb::SystemInfo`** — singleton with `getCPUCount`,
  `getOSVersion`, `getHostname`, `getMachineID`, memory sizes.
- **`cb::Directory` / `DirectoryWalker`** — directory iteration.
- **`cb::Glob`** — file globbing.
- **`cb::PowerManagement`** — battery / idle / keep-awake.
- **`cb::ProcessLock`** — single-instance file lock.
- **`cb::ProcessPriority`** — nice/IO priority hints.
- **`cb::SignalManager` / `ExitSignalHandler`** — graceful exit.
- **`cb::Pipe`** — bidirectional pipe (used internally by subprocesses).
- **`cb::DynamicLibrary`** — `dlopen`/`LoadLibrary` wrapper.

Header-by-header: everything lives under `cbang/os/` (cross-platform
parts) with `cbang/os/lin/`, `cbang/os/osx/`, `cbang/os/win/`
holding platform-specific implementations.

## Subprocess (synchronous)

```cpp
#include <cbang/os/Subprocess.h>

cb::Subprocess proc;
proc.exec({"ls", "-la", "/tmp"},
          cb::Subprocess::REDIR_STDOUT | cb::Subprocess::REDIR_STDERR);

int rc = proc.wait();        // blocks
LOG_INFO(1, "exit=" << rc);

// Read captured streams
std::string out;
proc.getStdOut()->read(out);
```

`exec` accepts an argv vector or a single command string (which
is split on whitespace; quote awareness varies, so the vector form
is safer for arbitrary inputs).

### Flags

| Flag | Effect |
|---|---|
| `REDIR_STDOUT` | Capture stdout to a pipe |
| `REDIR_STDERR` | Capture stderr to a pipe |
| `MERGE_STDERR` | Merge stderr into stdout |
| `NULL_STDOUT`/`NULL_STDERR` | Discard |
| `CLOSE_FDS` | Close all fds in child except 0/1/2 |
| `NEW_PROCESS_GROUP` | Child in its own process group |
| `CREATE_NO_WINDOW` | Suppress console window on Windows |
| `RAW_STDIO` | Don't wrap stdin/stdout/stderr |

Combine with `|`.  See `Subprocess.h` for the full list.

### Checking on a running child

```cpp
proc.exec({"long-job"}, 0);

while (proc.isRunning()) {
  // poll progress somewhere
  cb::Thread::sleep(1);
}
int rc = proc.wait();
```

### Killing

```cpp
proc.kill(SIGTERM);          // POSIX
proc.kill();                 // platform default
```

For Windows, this maps to `TerminateProcess`.

### Environment

```cpp
proc.setEnv("PATH", "/usr/local/bin:/usr/bin");
proc.exec({"my-tool"});
```

Set before `exec`.  Without explicit `setEnv`, the child inherits.

## Async subprocesses

For the event-loop world.  `AsyncSubprocess` raises an event when
the child exits; `SubprocessPool` manages a pool of them with a
ConcurrentPool-style API.

```cpp
#include <cbang/event/AsyncSubprocess.h>

cb::Event::AsyncSubprocess proc(base);
proc.exec({"slow-job"}, 0);

auto event = base.newEvent([&] (cb::Event::Event &, int, unsigned) {
  int rc = proc.wait();
  LOG_INFO(1, "exit=" << rc);
}, 0);
proc.onExit(event);
```

`SubprocessPool` (`cbang/event/SubprocessPool.h`) is the higher-level
form used by `cb::API` for handing off endpoint work to subprocess
workers.

## SystemUtilities — paths, files, directories

All free functions in the `cb::SystemUtilities` namespace.

### Path manipulation

```cpp
namespace SU = cb::SystemUtilities;

SU::basename("/a/b/c.txt");          // "c.txt"
SU::dirname("/a/b/c.txt");           // "/a/b"
SU::extension("c.txt");              // "txt"
SU::removeExtension("c.txt");        // "c"
SU::swapExtension("c.txt", "bak");   // "c.bak"
SU::isAbsolute("/a");                // true
SU::joinPath("a", "b");              // "a/b" (or "a\\b" on Windows)
SU::absolute("file");                // resolves against cwd
SU::getCanonicalPath("/a/./b/../c"); // "/a/c"
```

### Existence and type checks

```cpp
SU::exists("foo");
SU::isFile("foo");
SU::isDirectory("foo");
SU::isLink("foo");
SU::isDirectoryEmpty("foo");
```

### Directory ops

```cpp
SU::mkdir("/path/to/dir", /*withParents*/ true);
SU::rmdir("/path/to/dir", /*withChildren*/ true);   // recursive
SU::ensureDirectory("/path");                       // mkdir -p, idempotent

std::vector<std::string> entries;
SU::listDirectory(entries, "/path", /*listDirs*/ false);
```

### Reading and writing files

```cpp
std::string data = SU::read("config.txt");

SU::oopen("out.txt")->write(buffer);        // SmartPointer<ostream>
SU::iopen("in.bin");                         // SmartPointer<istream>

SU::unlink("temp.dat");
SU::rotate("log.txt", "", /*maxBackups*/ 16);
```

`rotate(path, dir, max)` archives `path` to a `dir` with a numeric
suffix, keeping at most `max` backups.  Used by the logger.

### cwd, executable, temp

```cpp
SU::getcwd();
SU::chdir("/some/path");
SU::getExecutablePath();              // path to this binary
SU::createTempDir("/tmp");            // unique subdir, returns path
```

### Working with paths in arguments

```cpp
auto tool = SU::findInPath("$PATH", "git");    // search PATH
auto paths = std::vector<std::string>{};
SU::splitPaths(getenv("LD_LIBRARY_PATH"), paths);
```

## SystemInfo

```cpp
#include <cbang/os/SystemInfo.h>

auto &si = cb::SystemInfo::instance();
uint32_t cpus = si.getCPUCount();
uint64_t mem  = si.getUsableMemory();
auto ver      = si.getOSVersion();
auto host     = si.getHostname();
auto machID   = si.getMachineID();
```

`getMachineID()` returns a stable per-machine identifier (e.g.
`/etc/machine-id` on Linux, hardware UUID on macOS, registry on
Windows).  fah-client uses this to detect machine moves and
trigger key regeneration (`App.cpp:437`).

`getPerformanceCPUCount()` returns the P-core count on hybrid
systems — fah-client uses this to default `--cpus` to the
performance-only count.

## Directory walking

```cpp
#include <cbang/os/DirectoryWalker.h>

cb::DirectoryWalker walker("/path", "*.cpp", /*depth*/ 0,
                           /*hidden*/ false);
while (walker.hasNext()) {
  std::string path = walker.next();
  // ...
}
```

For one-shot needs use `SystemUtilities::listDirectory`; for deep
traversal use `DirectoryWalker`.

## Glob

```cpp
#include <cbang/os/Glob.h>

cb::Glob g("/var/log/*.txt");
while (g.hasNext()) std::cout << g.next() << '\n';
```

## ProcessLock — single-instance enforcement

```cpp
#include <cbang/os/ProcessLock.h>

cb::ProcessLock lock("/var/run/myapp.pid");
// throws cb::Exception if another instance holds the lock
```

The lock is released when `lock` is destructed (process exit, scope
exit).

## ProcessPriority

```cpp
#include <cbang/os/ProcessPriority.h>

cb::ProcessPriority::setBelowNormal();
cb::ProcessPriority::setIdle();
cb::ProcessPriority::setNormal();
```

Maps to `nice` on POSIX and process priority class on Windows.

## PowerManagement

```cpp
#include <cbang/os/PowerManagement.h>

auto &pm = cb::PowerManagement::instance();
bool onBattery = pm.onBattery();
bool hasBattery = pm.hasSystemBattery();
bool idle       = pm.getSystemIdle();

pm.allowSystemSleep(false);     // keep the system awake
```

`allowSystemSleep(false)` issues OS-specific "keep awake" hints
(e.g. `caffeinate` on macOS, EXECUTION_STATE_DISPLAY on Windows,
inhibit_sleep on Linux).  fah-client uses this while a WU is
running (`OS.cpp:111`).

## SignalManager

`cb::SignalManager::instance().add(SIG..., handler)` lets you
install C-style signal handlers.  Prefer `Event::Base::newSignal`
for event-loop-driven apps; SignalManager is the bare-metal
mechanism.

`cb::ExitSignalHandler` (inherited by `Application`) installs
SIGINT/SIGTERM handlers that set a quit flag.

## Pipe and DynamicLibrary

`cb::Pipe` — bidirectional pipe, used internally by `Subprocess`
to wire stdio.  Rarely useful directly.

`cb::DynamicLibrary` — `dlopen` / `LoadLibrary` wrapper:

```cpp
cb::DynamicLibrary lib("libfoo.so");
auto *fn = (foo_func)lib.getSymbol("foo");
```

## Patterns

### Run a tool, capture output, parse JSON

```cpp
cb::Subprocess p;
p.exec({"jq", "-r", ".value", "config.json"},
       cb::Subprocess::REDIR_STDOUT);
if (p.wait() != 0) THROW("jq failed");

std::string out;
p.getStdOut()->read(out);
auto v = cb::JSON::Reader::parse(cb::InputSource(out));
```

### Atomic write

```cpp
std::string tmp = path + ".tmp";
SU::oopen(tmp)->write(data);
SU::rename(tmp, path);             // atomic on POSIX
```

### Ensure run directory

```cpp
SU::ensureDirectory("work");
SU::chdir("work");
```

### Detect machine ID changes

```cpp
auto current = cb::SystemInfo::instance().getMachineID();
auto stored  = db.getString("machine-id", "");

if (!stored.empty() && stored != current) {
  LOG_WARNING("Machine ID changed; regenerating identity");
  regenerate();
}
db.set("machine-id", current);
```

The fah-client pattern (`App.cpp:436-444`).

### Long-running CPU-bound work without blocking the loop

Either:
- Spawn a subprocess and watch with `AsyncSubprocess`.
- Use `Event::ConcurrentPool` to run on a worker thread.

Don't loop synchronously inside an event handler.

## Common pitfalls

- **`exec({"cmd arg1 arg2"})` vs `exec("cmd arg1 arg2")`.**  The
  vector form treats each element as an argv token.  The string
  form splits on whitespace (no quote handling).  Mixing them up
  produces "command not found" or argv-with-spaces bugs.
- **Reading stdout *before* the child exits.**  Pipes have OS
  buffer limits; if the child fills the pipe and you haven't
  drained, it blocks waiting.  Either drain in a thread/event, or
  use `MERGE_STDERR` and `getStdOut` with periodic reads.
- **`SystemInfo::instance().getMachineID()` failing.**  On some
  systems the ID isn't readable; wrap in `TRY_CATCH_ERROR`.
- **Calling `Subprocess::wait()` from the event loop.**  Blocks
  the whole loop.  Use the async variant.
- **`ProcessLock` not releasing.**  It releases on destructor, so
  scoped lifetime matters.  Putting it in a global is fine, in a
  short-lived scope is a bug.
- **`mkdir` without `withParents=true`** fails if the parent doesn't
  exist.  Use `ensureDirectory` for "mkdir -p" semantics.

## See also

- `cbang/os/Subprocess.h`, `cbang/os/SystemUtilities.h`,
  `cbang/os/SystemInfo.h`, `cbang/os/Directory.h`,
  `cbang/os/DirectoryWalker.h`, `cbang/os/Glob.h`,
  `cbang/os/ProcessLock.h`, `cbang/os/ProcessPriority.h`,
  `cbang/os/PowerManagement.h`, `cbang/os/SignalManager.h`,
  `cbang/os/ExitSignalHandler.h`, `cbang/os/DynamicLibrary.h`,
  `cbang/os/Pipe.h`.
- `cbang/event/AsyncSubprocess.h`,
  `cbang/event/SubprocessPool.h` for the async variants.
- [Application.md](Application.md) — Application owns
  ExitSignalHandler.
- [EventSystem.md](EventSystem.md) — `ConcurrentPool` for
  non-subprocess background work.
