# cbang Application Framework

`cb::Application` is the base class for a cbang program.  It wires
options parsing, logging, signal handling, config-file I/O, version
info, and the `printInfo` / `--help` / `--license` / `--info`
flags.  The pattern is: subclass `cb::Application`, register your
options in the constructor, do your work in `run()`, and let
`cb::doApplication<T>` be your `main`.

Every long-running cbang program — fah-client included — uses
this.

## Concepts

- **`cb::Application`** — your app's root object.  Owns `Options`,
  `Logger`, `CommandLine`.  Subclass to customise.
- **`cb::doApplication<T>(argc, argv)`** — template helper for
  `main`: initializes OpenSSL, constructs your subclass, calls
  `init`, calls `run`, prints any thrown exception.
- **Features** — a bitmask that enables/disables built-in flags
  (`--config`, `--print-info`, signal handling, debug options).
  Override `Application::_hasFeature` to suppress unwanted ones.
- **Lifecycle hooks** — `init`, `afterCommandLineParse`,
  `beforeDroppingPrivileges`, `run`, `printInfo`,
  `requestExit`, `shouldQuit`.

## Minimal example

```cpp
// myapp.cpp
#include <cbang/Application.h>
#include <cbang/ApplicationMain.h>

class MyApp : public cb::Application {
public:
  MyApp() : cb::Application("My App") {
    setVersion(cb::Version(1, 0, 0));

    options.pushCategory("Server");
    options.add("port", "Server port")->setDefault(8080);
    options.popCategory();
  }

  void run() override {
    LOG_INFO(1, "Listening on port " << options["port"].toInteger());
    // ... actual work ...
  }
};

int main(int argc, char *argv[]) {
  return cb::doApplication<MyApp>(argc, argv);
}
```

That's a complete cbang program.  Compile, link against `cbang`,
and you get:

- `--help`, `--info`, `--license`, `--version` for free
- Option parsing with type coercion and constraints
- XML/JSON config-file support (`--config FILE`)
- Logging through `LOG_*` macros (configured via `--verbosity`,
  `--log`, etc.)
- Clean signal handling (SIGINT/SIGTERM set `shouldQuit()` true)
- Stack-traced exception reporting on crash

## Lifecycle

```
cb::doApplication<T>(argc, argv)
  └─ SSL::init()
  └─ T app                            // ctor: register options
  └─ app.init(argc, argv)
       └─ Application::init()
            └─ CommandLine::parse()   // populate options from argv
            └─ openConfig()           // if --config passed
            └─ afterCommandLineParse()  // hook
            └─ beforeDroppingPrivileges()  // hook
            └─ logger.init()           // events / rotation
            └─ printInfo()  if --info or --print-info
            └─ return 0 (or negative if --help / --info exits early)
  └─ app.run()                         // your code
  └─ return 0
```

Override `init`, `afterCommandLineParse`,
`beforeDroppingPrivileges`, `run`, `printInfo`, or `usage` to
customise.

## Registering options

The constructor is the place.  By the time `init` is called, all
options must be registered so `--help` and the parser can see them.

```cpp
MyApp::MyApp() : Application("My App") {
  options.pushCategory("Networking");

  options.add("host", "Server hostname")->setDefault("localhost");
  options.add("port", "Server port",
              new cb::MinMaxConstraint<int32_t>(1, 65535))
         ->setDefault(8080);

  options.popCategory();
}
```

See [Config.md](Config.md) for the full options API.

## Reading options at run time

```cpp
void MyApp::run() {
  std::string host = options["host"];
  int64_t     port = options["port"].toInteger();
  // ...
}
```

## Hooks

| Hook | When | Use for |
|---|---|---|
| ctor | Object construction | Register options. |
| `init(argc, argv)` | Once at startup | Customise startup; call base class. |
| `afterCommandLineParse()` | Right after argv is parsed | Cross-option validation, derived state. |
| `beforeDroppingPrivileges()` | Before privilege drop | Open privileged files/sockets. |
| `run()` | After init | Your actual program. |
| `printInfo()` | On `--info` / `--print-info` | Print app-specific build/system info. |
| `requestExit()` | Signal received or programmatic | Trigger clean shutdown. |
| `shouldQuit()` | Polled from loops | Returns true after a request. |
| `openConfig(path)` | Internal | Override to customise config-file load. |

The base `init` is non-trivial — call `Application::init(argc,
argv)` from your override and respect its return value.

## Signal handling

`cb::ExitSignalHandler` (inherited) installs handlers for
`SIGINT`, `SIGTERM`, and (on platforms that have it) `SIGHUP`.  The
default behavior sets a flag readable via `shouldQuit()`.  In an
event-loop program, install your own signal events through
`Event::Base::newSignal` and call `requestExit()` from them — see
fah-client's `App` ctor for the canonical wiring.

`shouldQuit()` can be polled from anywhere as a "should I bail?"
predicate.  Loops that aren't driven by events should test it.

## Built-in command-line flags

`Application` registers these for you:

| Flag | Action |
|---|---|
| `--help` / `-h` | Print usage and exit 0. |
| `--help OPTION` | Detailed help for one option. |
| `--info` | Print system + app info and exit. |
| `--print` / `-d` | Print resolved config and exit. |
| `--json-help` | Print options as JSON. |
| `--license` | Show LICENSE text. |
| `--version` | Print version string. |
| `--config FILE` | Load options from FILE (XML or JSON). |
| `--quiet` / `-q` | Set verbosity to 0. |
| `--verbose` / `-v` | Bump verbosity (repeatable). |

A positional first arg is treated as a config file if you opt in:

```cpp
cmdLine.setAllowConfigAsFirstArg(true);   // in ctor or init
```

If your app forbids positional args:

```cpp
cmdLine.setAllowPositionalArgs(false);
```

(fah-client's App does both — `App.cpp:112-113`.)

## Config files

`openConfig(filename)` reads XML or JSON and applies values to
`Options`.  Called automatically when `--config` is used.  Errors
abort startup.

To save current settings back to disk:

```cpp
app.saveConfig("settings.xml");
```

`Application` also has a built-in **config rotation** feature: on
each save, the previous config gets archived to
`configRotateDir` with a numeric suffix, up to `configRotateMax`
entries.  Disable with `configRotate = false` if you don't want
this.

## `printInfo`

Override `printInfo()` to add your own sections to the
`--info` / `--print-info` output:

```cpp
void MyApp::printInfo() const {
  Application::printInfo();              // standard sections first
  LOG_INFO(1, "Custom: " << customData);
}
```

The base implementation already prints build info, system info, and
the running config.  fah-client uses `Info::instance().add(...)`
during construction to register additional sections.

## Working directory and privileges

`Application::init` chdirs to a configurable run directory if set
via `runDirectoryRegKey` (registry on Windows; see `App.cpp` for
the pattern).  It also drops setuid/setgid privileges after opening
files that require them.

`checkOpenFileLimit(N)` warns if your `RLIMIT_NOFILE` is below
`N`.  Useful for network servers.

## Multiple flags in a flag enum

The `Features` parent class lets the framework conditionally enable
parts of itself.  Subclass and override `_hasFeature` to disable
features you don't want:

```cpp
class MyApp : public cb::Application {
public:
  MyApp() : cb::Application("MyApp", _myHasFeature) {}

  static bool _myHasFeature(int feature) {
    if (feature == FEATURE_CONFIG_FILE) return false; // disable --config
    return Application::_hasFeature(feature);
  }
};
```

Available toggles: `FEATURE_CONFIG_FILE`, `FEATURE_DEBUGGING`,
`FEATURE_INFO`, `FEATURE_PRINT_INFO`, `FEATURE_SIGNAL_HANDLER`.

## Patterns

### Event-loop app

The typical cbang server pattern:

```cpp
class MyApp : public cb::Application {
  cb::Event::Base base{true, 10};
  cb::HTTP::Server server{base};
  cb::Event::EventPtr sigint;

public:
  MyApp() : cb::Application("MyApp") {
    // Register options ...

    auto exit = [this] { requestExit(); };
    (sigint = base.newSignal(SIGINT, exit))->add();
  }

  void run() override {
    server.init(options);
    base.dispatch();      // blocks until requestExit() → base.loopExit()
  }

  void requestExit() override {
    cb::Application::requestExit();
    base.loopExit();
  }
};
```

### Library-style app with `setup()` / `run()` split

For testability, factor `run()` into `setup()` + event-loop entry,
so test subclasses can construct your App, call `setup()`, drive
methods directly, and never enter the loop.  See fah-client's
`App::setup` (`App.cpp:529`) and `App::run` for the pattern.

### Verbose first-line banner

`Application::printInfo` emits the program banner with name,
version, copyright, build info, and system info.  This shows up by
default in your log file at startup, so you don't have to print it
yourself.

### Sub-applications / library use

If you need to embed an Application without taking over `main`,
construct it directly and call `init` / `run` yourself.  Skip
`doApplication` and handle exceptions in your caller.

## Common pitfalls

- **Registering options after `init`.**  `init` parses argv against
  whatever options exist *then*; later additions miss the CLI
  pass.  Register everything in the ctor.
- **Forgetting to call `Application::init`** from your override.
  You lose command-line parsing, signal handlers, logger setup,
  etc.
- **Doing real work in the ctor.**  The ctor runs before `init`;
  options aren't populated yet.  Real work goes in `run` (or
  `afterCommandLineParse` if it depends on options but must
  happen before privilege drop).
- **Throwing from `run()` and expecting a clean shutdown.**
  `doApplication` catches it but you've skipped any cleanup you
  needed.  Use try/catch in `run()` or RAII-based cleanup.
- **Polling `shouldQuit()` from a tight loop.**  Cheap, but if
  your loop is event-driven, install a signal handler on the
  Event::Base instead — it's more reactive.

## See also

- `cbang/Application.h`, `cbang/ApplicationMain.h`,
  `cbang/config/CommandLine.h`.
- [Config.md](Config.md) — option registration and reading.
- [Logging.md](Logging.md) — the logger is owned by Application.
- [EventSystem.md](EventSystem.md) — pairing Application with an
  Event::Base.
- [Exceptions.md](Exceptions.md) — error model used by
  `doApplication`.
- fah-client-bastet `App.cpp` ctor (option setup) and `App::run`
  (event-loop pattern), `src/client.cpp` (3-line main using
  `cb::doApplication<App>`).
