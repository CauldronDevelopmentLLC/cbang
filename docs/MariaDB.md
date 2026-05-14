# cbang MariaDB / MySQL

cbang's MariaDB binding wraps the MariaDB C client library
(`libmariadbclient`) and adds an event-driven async layer
(`cb::MariaDB::EventDB`) that runs all I/O through your
`cb::Event::Base`.  Use the sync `DB` for simple scripts and short
queries; use `EventDB` when MariaDB calls are part of a long-running
service that can't block.

## Concepts

- **`cb::MariaDB::DB`** — a single MariaDB connection.  Sync by
  default; non-blocking mode is available via `enableNonBlocking()`.
- **`cb::MariaDB::EventDB`** — `DB` subclass that drives the
  non-blocking handshake through the libevent loop and reports
  progress via a callback.
- **`cb::MariaDB::Field`** — typed accessor for one cell of one row.
- **`cb::MariaDB::QueryCallback`** — internal helper that runs a
  full async query lifecycle and re-emits events to your callback.
- **`cb::MariaDB::Connector`** — connection pool with retry / reuse
  on top of `EventDB`.

The protocol talks to MariaDB and MySQL interchangeably.

## Sync API (`DB`)

For one-off scripts, batch tools, or short-lived migrations.

```cpp
#include <cbang/db/maria/DB.h>

cb::MariaDB::DB db;

db.connect("db.example.com", "user", "secret", "mydb", 3306);

db.query("INSERT INTO users(name) VALUES('alice')");

db.query("SELECT id, name FROM users WHERE active = 1");
db.storeResult();                 // pull the result set client-side
while (db.fetchRow()) {
  int64_t     id   = db.getField(0).getS64();
  std::string name = db.getField(1).getString();
}
db.freeResult();
```

`fetchRow()` returns `false` at end-of-set.  `nextResult()` advances
to the next result set if `FLAG_MULTI_RESULTS` is enabled.

### Connection options

Set before `connect()`:

| Method | Effect |
|---|---|
| `setInitCommand(sql)` | SQL run on every connect |
| `enableCompression()` | Wire-level compression |
| `setConnectTimeout(s)` | Connection deadline |
| `setReadTimeout(s)` / `setWriteTimeout(s)` | I/O deadlines |
| `setProtocol(PROTOCOL_TCP/SOCKET/PIPE)` | Transport |
| `setReconnect(b)` | Auto-reconnect on idle drop |
| `setCharacterSet(name)` | e.g. `"utf8mb4"` |
| `setDefaultFile(path)` / `readDefaultGroup(name)` | Load my.cnf |
| `setLocalInFile(b)` | Enable `LOAD DATA LOCAL INFILE` |

Pass `flags` to `connect()` to enable/disable features
(`FLAG_DEFAULTS` enables multi-statements + multi-results).

### Parameterised queries

The MariaDB C API doesn't have prepared parameters at the wire level
in the way SQLite does.  cbang's helpers accept a JSON dict
parameter map and substitute into the query string.  See the
`EventDB::query(cb, sql, dict)` overloads below.

For unparameterised raw queries, escape input with
`mysql_real_escape_string` (exposed via the underlying `st_mysql *`
from `DB::getDB()`) or limit raw SQL to trusted inputs.

## Async API (`EventDB`)

The right choice for any cbang application that already runs on a
`cb::Event::Base`.  Non-blocking from connect to teardown: no
thread, no stall in the loop.

```cpp
#include <cbang/db/maria/EventDB.h>

cb::Event::Base       base;
cb::MariaDB::EventDB  db(base);

db.connect([&] (cb::MariaDB::EventDB::state_t state) {
  if (state == cb::MariaDB::EventDB::EVENTDB_DONE) {
    // Connected — start querying
    runFirstQuery();
  } else if (state == cb::MariaDB::EventDB::EVENTDB_ERROR) {
    LOG_ERROR("Connect failed: " << db.getError());
  }
}, "db.example.com", "user", "secret", "mydb");

base.dispatch();
```

The callback signature is `void(state_t)` and the same set of
states is used by every async method:

| State | Meaning |
|---|---|
| `EVENTDB_BEGIN_RESULT` | A result set is starting |
| `EVENTDB_ROW` | A row is ready; call `getField(i)` to read it |
| `EVENTDB_END_RESULT` | The result set is exhausted |
| `EVENTDB_RETRY` | Connection lost / transient error; retrying |
| `EVENTDB_DONE` | The operation is complete |
| `EVENTDB_ERROR` | Permanent failure; check `getError()` |

`EventDB` also has member-function overloads
(`connect(this, &Class::onConnect, ...)`) like the rest of cbang.

### Running a query

```cpp
db.query([&] (cb::MariaDB::EventDB::state_t state) {
  switch (state) {
  case cb::MariaDB::EventDB::EVENTDB_ROW:
    rows.emplace_back(
      db.getField(0).getS64(),
      db.getField(1).getString());
    break;
  case cb::MariaDB::EventDB::EVENTDB_DONE:
    onResults(std::move(rows));
    break;
  case cb::MariaDB::EventDB::EVENTDB_ERROR:
    onError(db.getError());
    break;
  default: break;
  }
}, "SELECT id, name FROM users WHERE active = 1");
```

The callback fires once per state transition.  `EVENTDB_ROW` fires
once per row in the result; read fields via `db.getField(i)` while
in that state.

### Parameterised queries with a JSON dict

```cpp
auto params = cb::JSON::build([] (cb::JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("min_id", 1000);
  sink.insert("active", true);
  sink.endDict();
});

db.query(cb, "SELECT * FROM users WHERE id >= ${min_id} AND active = ${active}",
         params);
```

The dict's keys are interpolated into `${name}` placeholders, with
correct quoting/escaping per the value's JSON type.

### Other operations

```cpp
db.ping(cb);                   // liveness check
db.close(cb);                  // graceful close (async)
```

## Reading fields

`getField(i)` returns a `cb::MariaDB::Field` that exposes typed
accessors:

```cpp
auto f = db.getField(0);

if (f.isNull()) ...;

auto t   = f.getType();          // INT, STRING, DECIMAL, BLOB, DATE, ...
auto i   = f.getS64();
auto u   = f.getU64();
auto d   = f.getDouble();
auto s   = f.getString();
auto raw = f.getBlob();
```

The `Field` is only valid for the current row; advance with the next
callback firing.

## Connector — pooling

For services that issue many queries, a single connection is a
bottleneck and a single point of failure.  `cb::MariaDB::Connector`
provides a small pool of `EventDB` connections, with retry on dead
connections.

```cpp
#include <cbang/db/maria/Connector.h>

cb::MariaDB::Connector pool(base);
pool.setOption("host",     "db.example.com");
pool.setOption("user",     "user");
pool.setOption("password", "secret");
pool.setOption("database", "mydb");
pool.init();

pool.query([&] (cb::MariaDB::EventDB::state_t state) {
  // same state machine as direct EventDB::query
}, "SELECT * FROM users", params);
```

The Connector hands each `query()` to a free connection (opening a
new one if needed, up to the pool max).  Tune via
`pool.setMaxConnections(n)` and friends.

## Threading

The async API is single-threaded — it runs on the same loop as
everything else.  The sync API is also not internally threaded; if
you must call it from multiple threads, give each thread its own
`DB` connection.  Don't share a `DB` instance across threads.

## Configuration via `cb::Application`

If your app uses cbang's options system, `Connector::addOptions()`
registers the standard MariaDB options on the command line:

```cpp
pool.addOptions(app.getOptions());
// --mariadb-host, --mariadb-user, --mariadb-password, --mariadb-database,
// --mariadb-port, --mariadb-socket, --mariadb-pool-size, ...
```

## Errors

The libmariadbclient error string is available via
`db.getError()`; the numeric code via `db.getErrorNumber()`.  The
async API forwards permanent errors as `EVENTDB_ERROR` and
recoverable ones as `EVENTDB_RETRY` (the next state will be the
real outcome).

Wrap sync calls in try/catch for `cb::Exception`.

## Patterns

### Query with bound parameters

```cpp
void Service::find(uint64_t id, std::function<void(...)> done) {
  auto params = cb::JSON::build([id] (cb::JSON::Sink &s) {
    s.beginDict();
    s.insert("id", id);
    s.endDict();
  });

  pool.query(WeakCall(this, [this, done] (auto state) {
    if (state == cb::MariaDB::EventDB::EVENTDB_ROW)  rows.push_back(...);
    if (state == cb::MariaDB::EventDB::EVENTDB_DONE) done(std::move(rows));
    if (state == cb::MariaDB::EventDB::EVENTDB_ERROR) onError(pool.getError());
  }), "SELECT * FROM users WHERE id = ${id}", params);
}
```

### Streaming a large result

Process rows as they arrive instead of accumulating:

```cpp
pool.query([&] (auto state) {
  if (state == cb::MariaDB::EventDB::EVENTDB_ROW)
    sink->writeRow(pool.getField(0).getString());
  if (state == cb::MariaDB::EventDB::EVENTDB_DONE)
    sink->end();
}, "SELECT name FROM big_table");
```

Each row callback fires from the event loop — no need to buffer the
whole result.

### Reconnect on transient failure

```cpp
db.setReconnect(true);     // libmariadbclient-level auto-reconnect
```

The async layer also reports `EVENTDB_RETRY` when it transparently
reconnects on a dropped connection.  Tune retries via
`QueryCallback`'s constructor parameter.

## Common pitfalls

- **Mixing sync and async on the same `DB`.**  Once you've called
  `enableNonBlocking()` (or used `EventDB`), the connection is
  managed by the loop; don't issue sync calls on it.
- **Reading fields outside `EVENTDB_ROW`.**  The row data is
  invalidated as soon as the next state fires.  Copy what you need
  during the row callback.
- **Sharing a connection across threads.**  Not safe.  Use a
  `Connector` or one `DB` per thread.
- **Ignoring `EVENTDB_RETRY`.**  Harmless if treated as
  intermediate — but if you maintain your own state machine on top,
  don't treat retry as failure.
- **Untrusted query strings.**  Use the JSON-dict parameter form;
  don't concatenate user input into SQL.

## See also

- `cbang/db/maria/DB.h`, `cbang/db/maria/EventDB.h`,
  `cbang/db/maria/Field.h`, `cbang/db/maria/QueryCallback.h`,
  `cbang/db/maria/Connector.h`.
- [EventSystem.md](EventSystem.md) — the loop the async client runs
  on.
- [JSON.md](JSON.md) — for the parameter dict.
- [SQLite.md](SQLite.md) — the local / embedded counterpart.
