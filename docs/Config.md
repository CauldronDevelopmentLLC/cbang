# cbang Config / Options

`cb::Options` is cbang's configuration system: a typed, named,
constraint-aware collection of values populated from command-line
flags, XML/JSON config files, environment, and programmatic setters.
When you derive from `cb::Application`, the Options machinery wires
itself up to `argv`, `--help`, and `--config FILE` automatically.

## Concepts

- **`cb::Options`** — a named set of options.  Indexed by string
  name.  Most apps have one global `Options` (owned by
  `cb::Application`); subsystems can also expose their own and merge.
- **`cb::Option`** — one option: name, type, default, constraint,
  current value, help text, optional action callback.
- **`cb::OptionType`** — `TYPE_BOOLEAN`, `TYPE_STRING`,
  `TYPE_INTEGER`, `TYPE_DOUBLE`, `TYPE_STRINGS`, `TYPE_INTEGERS`,
  `TYPE_DOUBLES` (the plural variants accept space-separated lists).
- **`cb::Constraint`** subclasses — validate values:
  `MinConstraint`, `MaxConstraint`, `MinMaxConstraint`,
  `RegexConstraint`, `EnumConstraint`.
- **`cb::OptionAction`** — callback fired when the value is set.
  Used for "do X when --refresh is passed."
- **`cb::Application`** integration — registers options, parses
  `argv` via `cb::CommandLine`, reads `--config`, prints `--help`,
  validates everything before `init()` returns.

## Registering options

If your class is an `Application` (or has access to its options):

```cpp
auto &opts = app.getOptions();

opts.pushCategory("Networking");

opts.add("host", "Server hostname")
    ->setDefault("localhost");

opts.add("port", "Server port",
         new cb::MinMaxConstraint<int32_t>(1, 65535))
    ->setDefault(8080);

opts.add("api-key", "API key for authentication")
    ->setObscured();                          // hidden from --help dump

auto opt = opts.add("backends", "Upstream backends (space-separated)");
opt->setType(cb::Option::TYPE_STRINGS);
opt->setDefault("api1.example.com api2.example.com");

opts.popCategory();
```

`add(name, help, [constraint])` returns a `SmartPointer<Option>`;
chain `setDefault`, `setType`, `setObscured` on it.

### Categories

`pushCategory("Name") ... popCategory()` groups related options in
the `--help` output and the config file.  Categories nest.  Always
pair push with pop.

### Short names and aliases

```cpp
opts.add("verbose", 'v', help);               // --verbose / -v
opts.alias("project-key", "key");             // --project-key sets "key"
```

Aliases share the same value: setting one sets the other.

### Constraints

| Constraint | Purpose |
|---|---|
| `MinConstraint<T>(min)` | Reject values below `min` |
| `MaxConstraint<T>(max)` | Reject values above `max` |
| `MinMaxConstraint<T>(min, max)` | Range check |
| `RegexConstraint(re, help)` | Reject if string doesn't match |
| `EnumConstraint<EnumT>` | Restrict to enum values |

Pass as the third arg to `add()`.  Examples from fah-client:

```cpp
opts.add("team", "Your team number",
         new cb::MinMaxConstraint<int32_t>(0, 2147483647))
    ->setDefault(0);

opts.add("passkey", "Your passkey",
         new cb::RegexConstraint(
           cb::Regex("[a-fA-F0-9]{32}"),
           "Passkey must be 32 hex characters."))
    ->setDefault("");
```

### Defaults

`setDefault(...)` accepts strings, integers, doubles, booleans, and
JSON values.  Picked the right typed overload automatically.  An
option without a `setDefault` has no value until something
explicitly sets one.

### Obscured options

`setObscured()` hides the value in `--info` / `--dump` output (still
visible in `--help`).  Use for credentials.

## Reading values

The recommended access patterns:

```cpp
auto &opts = app.getOptions();

bool        b   = opts["verbose"].toBoolean();
int64_t     n   = opts["port"].toInteger();
double      d   = opts["rate"].toDouble();
std::string s   = opts["host"];                     // implicit toString
auto        v   = opts["backends"].toStrings();     // vector<string>

if (opts["api-key"].hasValue())
  setupAPI(opts["api-key"]);
```

### `hasValue` vs `isDefault` vs `isSet`

| Method | True when |
|---|---|
| `hasValue()` | A value exists — either from set or default |
| `isSet()` | A value was explicitly set (CLI/file/programmatic) |
| `isDefault()` | The current value is the registered default |

Typical guard for "user actually provided this":

```cpp
if (opts["api-key"].isSet() && !opts["api-key"].isDefault())
  apply(opts["api-key"]);
```

### Bind to a member variable

```cpp
class Server {
public:
  unsigned port = 8080;
  std::string host;

  void registerOptions(cb::Options &opts) {
    opts.addTarget("host", host, "Server host")->setDefault("localhost");
    opts.addTarget("port", port, "Server port");
  }
};
```

`addTarget(name, var, help)` adds an option and wires up an action
that writes back to `var` whenever the option is set.  No manual
re-read needed.

## Actions

Callback fired when the value changes (via CLI, config file, or
programmatic set):

```cpp
opts.add("reload", 'r', this, &Self::doReload, "Reload configuration");
```

`doReload(Option &)` (the full form) or a parameterless member
function gets called whenever the option fires.  Common pattern for
flag-style switches that trigger work rather than store a value.

`addTarget` is implemented in terms of an action.

## Command-line parsing

If you derive from `cb::Application`, `init(argc, argv)` calls
`CommandLine::parse(argv)` which:

1. Walks `argv` matching against the registered options.
2. Honours `--option=value`, `--option value`, `-x value`, `-xy`
   (multiple short flags).
3. Treats the first non-flag arg as a config file path if
   `setAllowConfigAsFirstArg(true)` was set.
4. Triggers actions and constraint checks as each value is set.
5. Throws `cb::Exception` on unknown options or constraint failures.

Boolean options accept `--flag`, `--flag=true`, `--flag=1`,
`--flag=false`, `--flag=0`, and (via `--no-flag`) the negated form.

## Config files

`--config FILE` reads an XML or JSON file and applies options from
it.  Format:

```xml
<config>
  <host v="myserver"/>
  <port v="9000"/>
  <backends v="a.example.com b.example.com"/>
</config>
```

Or JSON:

```json
{ "host": "myserver", "port": 9000, "backends": "a b" }
```

Programmatic load:

```cpp
opts.read(cb::JSON::Reader::parseFile("config.json"));
```

The Options object is itself a `JSON::Serializable`, so you can
also dump it:

```cpp
cb::JSON::Writer w(std::cout);
opts.insert(w, /*config*/ true);          // emit JSON config view
```

This is what fah-client uses for `info.config` reflected to the
frontend.

## Iteration

```cpp
for (auto &[name, opt]: opts)
  std::cout << name << " = " << opt->toString() << '\n';
```

Options iterate in insertion order.

## Patterns

### Subsystem owns its options

A class registers its options inside its constructor:

```cpp
class Server {
public:
  Server(cb::Options &opts) {
    opts.pushCategory("Server");
    opts.add("listen", "Listen address")->setDefault("0.0.0.0:8080");
    opts.popCategory();
  }

  void init(cb::Options &opts) {
    SockAddr addr = SockAddr::parse(opts["listen"]);
    // ... bind ...
  }
};
```

Two-phase: ctor adds options so `--help` shows them; a separate
`init` reads them after parsing.

### Validate after parse

Constraints throw during parse, but cross-option checks happen
later:

```cpp
void App::run() {
  if (options["assignment-servers"].toStrings().empty())
    THROW("No assignment servers");
  // ...
}
```

### Reflect to JSON for a UI

```cpp
cb::JSON::Builder b;
options.insert(b, true);
ws.send(b.getRoot());
```

The "config" subtree of fah-client's observable state is just the
Options written out (`App::loadConfig`).

### Re-bind on the fly

```cpp
options.set("port", "9001");          // fires action, updates target
```

Equivalent to passing `--port=9001`.  Useful for hot-reload via
admin endpoint.

## Common pitfalls

- **Calling `add` after `init`.**  The option's value won't reflect
  whatever was in `argv`.  Register all options before
  `Application::init`.
- **Forgetting `popCategory`.**  Subsequent `add()` calls land in
  the wrong category in `--help`.
- **Reading an option that lacks a value.**  `toBoolean()` etc.
  throw on a missing value.  Use `hasValue()` or the
  `toBoolean(default)` overloads.
- **Mixing the alias and the canonical.**  Set one; read either.
  Setting both via CLI is fine but redundant.
- **Putting credentials in config without `setObscured()`.**  They
  appear in `--info` and `--dump` output.
- **Comparing `toString()` to the default literal.**  Use
  `isDefault()` instead — robust against CLI quoting and type
  coercion.

## See also

- `cbang/config/Options.h`, `cbang/config/Option.h`,
  `cbang/config/Constraint.h`, `cbang/config/MinMaxConstraint.h`,
  `cbang/config/RegexConstraint.h`, `cbang/config/CommandLine.h`.
- [Enumerations.md](Enumerations.md) — `EnumConstraint` ties an
  option to a cbang enum.
- [JSON.md](JSON.md) — config file I/O.
- [Logging.md](Logging.md) — `Logger::addOptions` adds the
  log-* family.
- fah-client-bastet `App.cpp` ctor — categories, constraints,
  aliases, obscured, target-binding.
