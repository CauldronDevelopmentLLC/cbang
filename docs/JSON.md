# cbang JSON

cbang's JSON module covers parsing, inspection, programmatic
construction, streaming output, and live observation of changes.  All
JSON values live in `cb::JSON::`.  The Folding@home client
(`fah-client-bastet`) uses every part of this API and is referenced
throughout.

## Concepts

- **`cb::JSON::Value`** — abstract base for every JSON value.
- **`cb::JSON::ValuePtr`** — `SmartPointer<Value>`, refcounted.  Pass
  values around by `ValuePtr`, not raw `Value &`.
- **Concrete types** — `Dict`, `List`, `Null`, `True`, `False`,
  `String`, `Number`, `Integer`.  You rarely name these directly; the
  factory/builder makes them.
- **`cb::JSON::Reader`** — strict or lenient JSON parser.
- **`cb::JSON::YAMLReader`** — YAML 1.1 parser that emits the same tree.
- **`cb::JSON::Writer`** — pretty/compact serializer to an `ostream`.
- **`cb::JSON::Sink`** — abstract streaming writer interface.  A
  `Writer` is a `Sink`; so is a `Builder`; so are several others.
- **`cb::JSON::Builder`** — `Sink` that builds an in-memory `ValuePtr`.
- **`cb::JSON::ObservableDict` / `ObservableList`** — mutable values
  that fire `notify()` on every change.  Used as the live state tree
  in event-driven apps.

Two complementary patterns:
- **In-memory:** `parse()` → `ValuePtr` → inspect/mutate → `toString()`.
- **Streaming:** parse/build directly through a `Sink`, never
  materializing the whole tree.  Useful for large bodies and for
  feeding output to an HTTP response without an intermediate buffer.

## Parsing

### From a string, stream, or file

```cpp
#include <cbang/json/Reader.h>

auto v = JSON::Reader::parse(std::cin);         // istream
auto v = JSON::Reader::parse("/path/to.json");  // wrong — InputSource
auto v = JSON::Reader::parseFile("/path/to.json");
auto v = JSON::Reader::parse(InputSource(str)); // explicit
```

`Reader::parse` throws on malformed input; the exception carries line
and column.  Use `strict=true` for spec-strict mode (default is
permissive: trailing commas, comments).

### YAML

```cpp
#include <cbang/json/YAMLReader.h>

auto v = JSON::YAMLReader::parseFile("config.yaml");
```

Both readers can also stream into a `Sink` instead of building a tree;
see `parse(InputSource, Sink&)`.

## Inspecting

Every getter has three useful forms: typed (`getU32`, `getString`,
…), with-default, and `has*`/`exists*` predicates.

```cpp
JSON::ValuePtr v = ...;  // {"cpus": 4, "user": "alice", "gpus": []}

bool        ok    = v->isDict();
unsigned    cpus  = v->getU32("cpus");
std::string user  = v->getString("user", "Anonymous");   // default
bool        hasK  = v->hasU32("cpus");
unsigned    sz    = v->getList("gpus").size();
```

### Typed accessors (X-macro generated)

For each scalar type there are three families:

| Pattern | Throws when | Example |
|---|---|---|
| `get<T>(key)` | key missing or wrong type | `v->getU32("cpus")` |
| `get<T>(key, default)` | never | `v->getU32("cpus", 1)` |
| `has<T>(key)` | n/a | `v->hasU32("cpus")` |

The full set of `<T>`:
```
Dict List Undefined Null Boolean Number String
S8 U8 S16 U16 S32 U32 S64 U64
```

Positional accessors exist too (for lists): `v->get(0)`,
`v->getU32(0)`.

### Dotted paths

`Path` lets you walk into nested structures without manual `get` chains:

```cpp
v->select("assignment.data.cpus")       ->getU32();
v->selectU32("assignment.data.cpus");                  // shorthand
v->selectU32("assignment.data.cpus", 1);               // default
v->selectString("request.data.user", "Anonymous");
v->existsString("request.data.passkey");
```

`select` returns null if any step is missing (unless you pass a
default).  Used heavily in fah-client — see `Unit::getCreditEstimate`
(`Unit.cpp:305`) which selects six paths into the assignment-server
response.

### Iteration

```cpp
for (auto &v: *list)        std::cout << *v << '\n';            // list items
for (auto &k: dict->keys()) std::cout << k << '\n';             // dict keys
for (auto &e: dict->entries()) {                                 // (key,value)
  std::cout << e.first << " = " << *e.second << '\n';
}
```

Iterators are stable across `merge`/`insert` of unrelated keys but
invalidated by `erase` of the same key.

### Type checks and errors

`get*` throws `cb::Exception` (specifically `TypeError` or `KeyError`)
when the value is the wrong type or the key is missing.  Prefer
`get*WithDefault` or `has*` checks when missing-or-wrong is expected.

## Building

### `Builder` / `JSON::build`

For one-shot construction, the easiest pattern is `JSON::build`:

```cpp
#include <cbang/json/Builder.h>

auto v = JSON::build([] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("user", "alice");
  sink.insert("team", 42u);
  sink.beginInsert("gpus");
    sink.beginList();
      sink.append("gpu:0");
      sink.append("gpu:1");
    sink.endList();
  sink.endDict();
});

std::cout << *v;
```

The lambda receives a `Sink`; on exit, `build` returns the populated
`ValuePtr`.  This is how fah-client constructs the assignment-server
request payload in `Unit::assign` (`Unit.cpp:1062`).

### Direct value construction

```cpp
ValuePtr d = new JSON::Dict;
d->insert("k", "v");
d->insert("n", 42);

ValuePtr l = new JSON::List;
l->append("a");
l->append(1);

d->insert("items", l);
```

For observable trees, use `Observable<Dict>` / `Observable<List>` (or
their typedefs `ObservableDict` / `ObservableList`).

### Convenience factories

`Value` provides `createDict()` / `createList()` / `createString(...)`
that return values of the same kind as the receiver.  An
`ObservableDict::createDict()` returns an `ObservableDict`, not a
plain `Dict`.  Use these when building from inside an observable tree
to keep the whole subtree observable.

## Mutating

```cpp
d->insert("cpus", 4);                          // typed insert
d->insertBoolean("paused", true);
d->insertList("gpus");                         // empty list
d->erase("temp");

d->merge(*otherDict);                          // shallow merge

// On a List:
l->append(value);
l->set(i, value);
l->erase(i);
l->clear();
```

Updates fire `notify()` on observable values (see below).

## Writing

### To a stream

```cpp
#include <cbang/json/Writer.h>

JSON::Writer w(std::cout);                     // pretty, indent 2
v->write(w);

JSON::Writer compact(std::cout, 0, true);      // compact (no whitespace)
v->write(compact);
```

Or directly:

```cpp
v->write(std::cout, /*indentStart*/ 0, /*compact*/ false,
         /*indentSpace*/ 2, /*precision*/ 6);
```

### To a string

```cpp
std::string s = v->toString();                 // pretty
std::string c = v->toString(0, true);          // compact
```

### To an HTTP response

`cb::HTTP::Request::reply` and `send` accept a sink-callback:

```cpp
req.reply(HTTP_OK, [&] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("ok", true);
  sink.insert("count", n);
  sink.endDict();
});
```

This streams directly to the socket buffer — no intermediate
serialization to a string.  See [WebServer.md](WebServer.md).

## Sinks in depth

A `Sink` is the unified write interface.  Producers (parser, builder,
existing value via `Value::write(sink)`) talk to it; consumers
(in-memory builder, stream writer, observable tree) implement it.
This decoupling means parsing into a builder, serializing a value to
an HTTP response, and feeding YAML into a JSON Writer all use the
same vocabulary.

The core operations:

| Operation | Behavior |
|---|---|
| `writeNull()`, `writeBoolean(b)`, `write(n)`, `write(s)` | Emit a scalar at the current cursor. |
| `beginDict()` / `endDict()` | Start/end a dict.  Inside, use `beginInsert(key)` then a scalar/begin-list/begin-dict. |
| `beginList()` / `endList()` | Start/end a list.  Inside, use `beginAppend()` then a scalar/begin-list/begin-dict, or the `append(value)` shortcut. |
| `insert(key, value)` / `append(value)` | Shortcuts that combine `beginInsert`/`beginAppend` and `write`. |
| `getDepth()` | Current nesting depth, useful for sanity checks. |

Several specialised sinks ship with cbang:

- **`Builder`** — accumulates a `ValuePtr`.
- **`Writer`** / **`BufferWriter`** — serialize to a stream / a libevent buffer.
- **`NullSink`** — discards everything; base class.
- **`TeeSink`** — fan out to two sinks (e.g. write *and* build).
- **`ProxySink`** — forward to another sink; subclass to intercept.
- **`YAMLMergeSink`** — implements YAML's merge keys.
- Observable values double as sinks: `dict->insert(key, value)` and
  the streaming sink methods both fire `notify`.

## Observable values

`ObservableDict` and `ObservableList` (specializations of
`Observable<T>`) emit a change list on every mutation.  Override
`notify(const std::list<ValuePtr> &change)` to react.

```cpp
class MyState : public JSON::ObservableDict {
public:
  void notify(const std::list<JSON::ValuePtr> &change) override {
    // `change` is a path-then-value list:
    //   ["units", "abc", "state"]  <new-value>
    // The new value is the last element.
    auto path = change;
    auto newValue = path.back(); path.pop_back();
    for (auto &p: path) std::cout << '/' << p->asString();
    std::cout << " = " << *newValue << '\n';
  }
};
```

The change format is a flat list: each entry is the key (string for
dict, integer for list) of one step into the tree, with the final
entry being the new value (or absent for deletions).  fah-client's
`App` is an `ObservableDict`; its `notify` forwards every change to
attached frontends (`App::notify`, `App.cpp:550`).

Important rules:
- Observable values must contain only observable children, or
  notifications won't propagate.  The factory methods on observable
  values (`createDict`, `createList`) return observable children; use
  those when building observable subtrees.
- Don't store raw `Value *` to observable children — use `ValuePtr`.

## Numeric types

JSON has only "number", but cbang preserves the underlying type when
it can.  Use the most specific getter you have:

```cpp
v->getU32("count");        // uint32_t
v->getS64("offset");       // int64_t
v->getNumber("rate");      // double
```

Round-trip: parsing an integer yields a `JSON::Integer` (preserving
range up to 64 bits); parsing a number with a decimal or exponent
yields a `JSON::Number` (`double`).

## Errors

Most type/path mistakes throw `cb::Exception`:

- `getU32("missing")` — throws `KeyError`.
- `getU32("user")` when `user` is a string — throws `TypeError`.

Two ways to handle:
1. Use `get*WithDefault` or the `(key, default)` overload.
2. Wrap with `TRY_CATCH_ERROR(...)` / `CATCH_ERROR` macros.

When parsing fails, `Reader` throws with the line and column.

## End-to-end examples

### Build a request payload (from `Unit::assign`)

```cpp
auto request = JSON::build([this] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("time",    Time().toString());
  sink.insert("user",    config->getUsername());
  sink.insert("team",    config->getTeam());

  sink.beginInsert("resources");
  sink.beginDict();
    sink.beginInsert("cpu");
    sink.beginDict();
      sink.insert("cpus",   cpuInfo->getCount());
      sink.insert("vendor", cpuInfo->getVendor());
    sink.endDict();
  sink.endDict();

  sink.endDict();
});
```

### Parse and inspect a response

```cpp
auto msg = req.getInputJSON();        // from HTTP::Request

if (msg->existsDict("error"))
  THROW("Server returned: " << msg->selectString("error.message"));

unsigned cpus    = msg->selectU32   ("assignment.data.cpus");
uint64_t timeout = msg->selectU64   ("assignment.data.timeout", 0);
auto     wsHost  = msg->selectString("assignment.data.ws");
```

### Serialize a value to disk

```cpp
std::ofstream f("snapshot.json");
v->write(f);
```

### Stream an HTTP JSON response without buffering

```cpp
req.reply(HTTP_OK, [&] (JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("status", "ok");
  sink.beginInsert("results");
  sink.beginList();
  for (auto &r: results) {
    sink.beginAppend();
    sink.beginDict();
    sink.insert("id",    r.id);
    sink.insert("value", r.value);
    sink.endDict();
  }
  sink.endList();
  sink.endDict();
});
```

### Observe live state changes

```cpp
class App : public JSON::ObservableDict {
public:
  void notify(const std::list<JSON::ValuePtr> &change) override {
    for (auto &remote: remotes)
      remote->sendChanges(change);
  }
};
```

See `App::notify` (`App.cpp:550`) for the production version that
filters config-saves and forwards to attached remotes.

## See also

- `Value.h`, `Reader.h`, `Builder.h`, `Writer.h`, `Sink.h`,
  `Observable.h` for the full API surface.
- `YAMLReader.h` for YAML input.
- `Path.h` for dotted-path semantics.
- [WebServer.md](WebServer.md) — integrating JSON with HTTP.
- fah-client-bastet for production examples — particularly `App.cpp`
  (observable state tree), `Unit.cpp` (build/parse, signed payloads),
  `Config.cpp` (defaults + merge + typed inserts).
