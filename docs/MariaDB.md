# cbang MariaDB / MySQL

cbang's MariaDB binding wraps the MariaDB C client library
(`libmariadbclient`) with an event-driven async layer that runs all I/O
through your `cb::Event::Base`.  Queries execute as **prepared statements**
(`mysql_stmt_*`) over the binary protocol, with support for bound
parameters — including binary blobs.

The query path is non-blocking only; it is designed to be driven from the
event loop, normally via `EventDB`.  Connection management has both blocking
and non-blocking variants.

## Concepts

- **`cb::MariaDB::DB`** — a single connection plus one prepared statement.
  Blocking and non-blocking (`*NB`) connection methods; non-blocking
  query/result methods.
- **`cb::MariaDB::EventDB`** — `DB` subclass that drives the non-blocking
  calls through the libevent loop and reports progress via a callback.
  This is the class you normally use.
- **`cb::MariaDB::Field`** — column metadata (name, type, width) for the
  current result set.
- **`cb::MariaDB::QueryCallback`** — internal state machine that runs one
  query lifecycle (prepare → bind → execute → result sets → rows) and
  re-emits events to your callback.
- **`cb::MariaDB::Connector`** — connection factory: configures and starts
  a fresh non-blocking `EventDB` per `getConnection()`.

The protocol talks to MariaDB and MySQL interchangeably.

## Connection

Connection methods come in blocking and non-blocking pairs: `connect` /
`connectNB`, `ping` / `pingNB`, `close` / `closeNB`, `use` / `useNB`,
`changeUser` / `changeUserNB`, `resetConnection` / `resetConnectionNB`.
The `*NB` forms return `false` when the operation is pending; completion is
driven via `continueNB()` (see Non-blocking driving below) — or use
`EventDB`, which does this for you.

### Options

Set before connecting:

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
| `enableNonBlocking()` | Required before any `*NB` call |

Pass `flags` to `connect()` to enable/disable protocol features
(`FLAG_DEFAULTS` enables multi-statements + multi-results).

## Async API (`EventDB`)

The right choice for any cbang application that runs on a
`cb::Event::Base`.  Non-blocking from connect to teardown.

```cpp
#include <cbang/db/maria/EventDB.h>

cb::Event::Base      base;
cb::MariaDB::EventDB db(base);

db.enableNonBlocking();
db.connect([&] (cb::MariaDB::EventDB::state_t state) {
  if (state == cb::MariaDB::EventDB::EVENTDB_DONE) runFirstQuery();
  else LOG_ERROR("Connect failed: " << db.getError());
}, "db.example.com", "user", "secret", "mydb");

base.dispatch();
```

The callback signature is `void (state_t)` and the same states are used by
every async method:

| State | Meaning |
|---|---|
| `EVENTDB_BEGIN_RESULT` | A result set is starting |
| `EVENTDB_ROW` | A row is ready; read it now |
| `EVENTDB_END_RESULT` | The result set is exhausted |
| `EVENTDB_RETRY` | Deadlock detected; the query is being retried |
| `EVENTDB_DONE` | The operation completed |
| `EVENTDB_ERROR` | Failure; check `getError()` |

`EventDB` also has member-function overloads
(`connect(this, &Class::onConnect, ...)`) like the rest of cbang.

### Running a query

```cpp
db.query([&] (cb::MariaDB::EventDB::state_t state) {
  switch (state) {
  case cb::MariaDB::EventDB::EVENTDB_ROW:
    rows.emplace_back(db.getS64(0), db.getString(1));
    break;
  case cb::MariaDB::EventDB::EVENTDB_DONE:  onResults(rows);          break;
  case cb::MariaDB::EventDB::EVENTDB_ERROR: onError(db.getError());   break;
  default: break;
  }
}, "SELECT id, name FROM users WHERE active = 1");
```

`EVENTDB_ROW` fires once per row; read fields during that state.  The query
runs as a prepared statement: prepare, bind, execute, then rows stream over
the binary protocol.

### Bound parameters

Parameters bind to `?` placeholders in order — binary-safe, any content,
no quoting or escaping:

```cpp
std::vector<std::string> params = {imageBytes};
db.query(cb, "CALL StoreImage(?, 'image/png')", params);
```

This is also how the API framework binds `{body}` / `{files.*}` blob refs
(see [API.md](API.md)).

### Interpolated parameters

For non-binary values there is also string interpolation from a JSON dict:
`{name}` references are replaced with the dict value, quoted and escaped per
its JSON type (strings quoted, numbers and booleans bare, null → `NULL`):

```cpp
cb::JSON::ValuePtr dict = new cb::JSON::Dict;
dict->insert("min_id", 1000);

db.query(cb, "SELECT * FROM users WHERE id >= {min_id}", dict);
```

### Other operations

```cpp
db.ping(cb);                   // liveness check
db.close(cb);                  // graceful close (async)
```

## Reading rows

Typed cell accessors live on `DB` and apply to the current row:

```cpp
db.isNull(i);
db.getString(i);               // binary-safe, keeps length
db.getS64(i);  db.getU64(i);  db.getDouble(i);  db.getBoolean(i);
db.getJSON(i);                 // cell as a typed JSON value
db.writeRowDict(sink);         // whole row into a JSON sink
db.getRowList();  db.getRowDict();
```

`getField(i)` returns column *metadata* (`getName()`, `getType()`,
`isNumber()`, ...).  Row data is only valid during `EVENTDB_ROW`; copy what
you need before returning.

## Connector

`cb::MariaDB::Connector` turns connection settings (or command-line
options) into ready-to-use non-blocking connections:

```cpp
#include <cbang/db/maria/Connector.h>

cb::MariaDB::Connector connector(base);
connector.addOptions(options);   // --db-host, --db-user, --db-pass,
                                 // --db-name, --db-port, --db-timeout

auto db = connector.getConnection();   // a fresh, connecting EventDB
db->query(cb, "SELECT 1");
```

Each `getConnection()` returns a new `EventDB` already configured
(timeouts, reconnect, `utf8mb4`, non-blocking) with `connectNB()` started;
queueing a query on it completes once the connection is up.

## Non-blocking driving (low level)

If you drive `DB` yourself instead of using `EventDB`: every `*NB` method
returns `false` when the operation would block.  Wait for the socket
(`getSocket()`, `waitRead()` / `waitWrite()` / `waitTimeout()`), then call
`continueNB(ready)` until it returns `true`.  The query lifecycle is
`queryNB(sql, params)` → `storeResultNB()` → `fetchRowNB()` /
`haveRow()` → `freeResultNB()` → `moreResults()` / `nextResultNB()`.

## Threading

The async API is single-threaded — it runs on the same loop as everything
else.  Don't share a `DB` across threads; create one per thread or use
per-loop connections from a `Connector`.

## Errors

`getError()`, `getErrorNumber()` and `getSQLState()` report the statement
error if one is active, else the connection error.  The async layer
forwards failures as `EVENTDB_ERROR` and retries deadlocks
(`EVENTDB_RETRY`, then the real outcome).  All methods throw
`cb::Exception` on hard failures.

## Common pitfalls

- **Reading fields outside `EVENTDB_ROW`.**  Row data is invalidated when
  the next state fires.  Copy during the row callback.
- **Sharing a connection across threads.**  Not safe.
- **Ignoring `EVENTDB_RETRY`.**  Treat it as intermediate; don't count it
  as failure in your own state machine.
- **Untrusted input in SQL strings.**  Use `?` bound parameters or the
  JSON-dict interpolation, which quotes and escapes; never concatenate.
- **Forgetting `enableNonBlocking()`** before `*NB` calls on a raw `DB`
  (`EventDB` via `Connector` is already configured).

## See also

- `cbang/db/maria/DB.h`, `cbang/db/maria/EventDB.h`,
  `cbang/db/maria/Field.h`, `cbang/db/maria/QueryCallback.h`,
  `cbang/db/maria/Connector.h`.
- [EventSystem.md](EventSystem.md) — the loop the async client runs on.
- [API.md](API.md) — the declarative API framework built on this layer.
- [JSON.md](JSON.md) — for the parameter dict and row sinks.
