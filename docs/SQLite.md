# cbang SQLite

cbang wraps SQLite into a small, ergonomic C++ API: open/close, raw
SQL exec, prepared statements with bound parameters, transactions,
backups, and a convenience `NameValueTable` for the common
key/value-with-extras pattern.  All synchronous — SQLite is fast
enough for typical configuration and bookkeeping that you call it
directly from the event loop.

## Concepts

- **`cb::DB::Database`** — one open SQLite connection.
- **`cb::DB::Statement`** — a prepared, parameterised query.
- **`cb::DB::Column` / `cb::DB::Parameter`** — typed accessors for
  one column of a result row / one parameter on a prepared
  statement.
- **`cb::DB::Transaction`** — RAII transaction handle.  Commits on
  destruction unless you `rollback()`.
- **`cb::DB::NameValueTable`** — pre-baked `(name TEXT PRIMARY KEY,
  value, ts?)` table with typed get/set/foreach helpers.
- **`cb::DB::Backup`** — online backup to another `Database`.

cbang also has `cb::DB::EventLevelDB` for LevelDB-backed key/value
stores; this doc covers the SQLite path.

## Open / close

```cpp
#include <cbang/db/Database.h>
#include <cbang/db/Statement.h>

cb::DB::Database db;
db.open("client.db");                 // creates if missing

// recommended PRAGMAs for normal apps
db.execute("PRAGMA journal_mode=WAL");
db.execute("PRAGMA locking_mode=EXCLUSIVE");
db.execute("PRAGMA synchronous=NORMAL");
db.execute("PRAGMA auto_vacuum=FULL");
```

The constructor takes an optional busy timeout (default 30 s).
Flags to `open()` (default `READ_WRITE | CREATE`):

| Flag | Meaning |
|---|---|
| `READ_ONLY` | Open existing DB read-only |
| `READ_WRITE` | Open for read/write |
| `CREATE` | Create if missing (only with `READ_WRITE`) |
| `NO_MUTEX` | No SQLite-level mutex (we manage threading) |
| `FULL_MUTEX` | Serialize all access (multi-thread safe but slower) |
| `SHARED_CACHE` / `PRIVATE_CACHE` | Page cache mode |

`close()` is called by the destructor; manual close is rarely
needed.

## Quick exec — no parameters, simple result

```cpp
db.execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT)");

int64_t count = 0;
db.execute("SELECT COUNT(*) FROM users", count);

std::string name;
db.execute("SELECT name FROM users WHERE id=1", name);
```

The `execute(sql, out)` overloads pull a single value from the first
result row.  Return `false` if the query produced no row.

`executef(fmt, ...)` is a printf-style helper — useful for ad-hoc
statements but **never use it with untrusted input**; it doesn't
escape values.  Use prepared statements with bound parameters
instead.

## Prepared statements

```cpp
auto stmt = db.compile("INSERT INTO users (id, name) VALUES (?1, ?2)");
stmt->parameter(1).bind((int64_t)42);
stmt->parameter(2).bind(std::string("alice"));
stmt->execute();

stmt->reset();
stmt->clearBindings();
stmt->parameter(1).bind((int64_t)43);
stmt->parameter(2).bind(std::string("bob"));
stmt->execute();
```

`compile()` and `compilef()` return a `SmartPointer<Statement>`.
Cache them as instance fields when you'll re-run a query many times.

Parameters can be referenced by position (`?N`) or by name (`:name`,
`@name`, `$name`).  Get them with `stmt->parameter(N)` or
`stmt->parameter("name")`.

### Reading result rows

```cpp
auto stmt = db.compile("SELECT id, name FROM users WHERE name LIKE ?1");
stmt->parameter(1).bind(std::string("a%"));

while (stmt->next()) {
  int64_t     id   = stmt->column(0).asInt64();
  std::string name = stmt->column(1).asString();
  // ...
}
```

`next()` returns `true` if a row was fetched, `false` at end-of-set.
`column(i)` is valid until the next `next()` call or statement
destruction.

`Column::getType()` returns the SQLite type (`INTEGER`, `FLOAT`,
`TEXT`, `BLOB`, `NULL`).  The `as*` accessors coerce; use
`getType()` first if you need to distinguish null from zero.

### One-shot result via execute

For statements that return exactly one row of one column, use the
typed `execute(out)` overloads on the statement (mirroring the
Database-level helpers).

## Transactions

```cpp
{
  auto tx = db.begin();                              // DEFERRED
  db.execute("INSERT INTO users(name) VALUES('alice')");
  db.execute("INSERT INTO users(name) VALUES('bob')");
  tx->commit();
}                                                   // rollback if not committed
```

If `tx` goes out of scope before `commit()`, the transaction is
rolled back.  This is the safe default.

The transaction type maps to the SQLite literal:

| Enum | Meaning |
|---|---|
| `DEFERRED` | Default — locks acquired on first read/write |
| `IMMEDIATE` | RESERVED lock taken at begin |
| `EXCLUSIVE` | EXCLUSIVE lock taken at begin (no other readers/writers) |

Use `IMMEDIATE` or `EXCLUSIVE` when you need to be sure of write
access before doing anything that depends on it.

You can also `db.commit()` / `db.rollback()` directly without the
RAII handle, but the handle pattern is preferred.

## NameValueTable

The most common SQLite usage in cbang apps is a "name → value" table
with optional timestamps and JSON values.  `NameValueTable` provides
the typed accessors so you don't write the same five queries by
hand:

```cpp
cb::DB::NameValueTable kv(db, "config");
kv.create();                                  // CREATE TABLE IF NOT EXISTS ...
kv.init();                                    // prepare cached statements

kv.set("user",     std::string("alice"));
kv.set("count",    (int64_t)42);
kv.set("rate",     1.5);
kv.set("enabled",  true);
kv.set("config",   *jsonValue);               // JSON value

bool        on    = kv.getBoolean("enabled", false);
int64_t     count = kv.getInteger("count");
auto        cfg   = kv.getJSON("config");
std::string name  = kv.getString("user", "anonymous");

if (kv.has("user")) ...;
kv.unset("count");

kv.foreach([] (const std::string &name, const std::string &value) {
  std::cout << name << " = " << value << '\n';
}, /*limit*/ 0);
```

Constructor signature: `NameValueTable(db, table, ordered=false)`.
`ordered=true` adds a `ts` column with default `CURRENT_TIMESTAMP`
and an index on it — enables ordered iteration.

This is how fah-client persists config, group state, units, and the
WU log.  `App::getDB(name, ordered)` (`App.cpp:225`) lazily creates
and caches per-table instances.

## JSON helpers

`Statement` can write its result rows directly into a JSON sink:

```cpp
auto stmt = db.compile("SELECT id, name, ts FROM events WHERE ts > ?");
stmt->parameter(1).bind((int64_t)since);

cb::JSON::Writer w(std::cout);
stmt->readAll(w);
// emits a JSON list of dicts {id, name, ts}
```

`readOne(sink)` reads the next row only; `readHeader(sink)` writes
the column names as a list.

`NameValueTable::getJSON(name)` deserializes a stored string back to
a `JSON::Value`.

## Backup

Online backup to another database (typically used to snapshot to
disk or to a fresh file):

```cpp
cb::DB::Database backupDb;
backupDb.open("snapshot.db");

auto backup = db.backup(backupDb);
while (backup->step(100)) {     // page-at-a-time; cooperative
  // optionally poll progress: backup->getRemaining(), backup->getPageCount()
}
```

## Errors

Most methods throw `cb::Exception` on failure.  The exception
message includes the SQLite error string.  `db.lastError()` and
`db.lastErrorMsg()` are available for explicit checks.

For non-throwing variants, use the `execute(sql, out)` overloads
that return `bool`.

## Patterns

### Lazy per-table cache

```cpp
class App {
  cb::DB::Database db;
  std::map<std::string, cb::SmartPointer<cb::DB::NameValueTable>> tables;

public:
  cb::DB::NameValueTable &getTable(const std::string &name, bool ordered = false) {
    auto it = tables.find(name);
    if (it == tables.end()) {
      auto t = cb::SmartPtr(new cb::DB::NameValueTable(db, name, ordered));
      t->create();
      t->init();
      it = tables.emplace(name, t).first;
    }
    return *it->second;
  }
};
```

Matches `App::getDB` in fah-client.

### Schema migration

```cpp
auto version = kv.getInteger("schema-version", 0);
if (version < 1) { db.execute("..."); kv.set("schema-version", 1); }
if (version < 2) { db.execute("..."); kv.set("schema-version", 2); }
```

The `App::upgradeDB` pattern in fah-client.

### Bulk insert in a transaction

```cpp
auto tx   = db.begin(cb::DB::Database::IMMEDIATE);
auto stmt = db.compile("INSERT INTO log(name, value) VALUES(?1, ?2)");

for (auto &entry: batch) {
  stmt->reset();
  stmt->parameter(1).bind(entry.name);
  stmt->parameter(2).bind(entry.value);
  stmt->execute();
}

tx->commit();
```

Order of magnitude faster than implicit per-statement transactions.

### Storing JSON

```cpp
kv.set("config", *configValue);            // serializes to a TEXT row
auto reloaded = kv.getJSON("config");      // parses back to ValuePtr
```

`NameValueTable` handles the to/from-string for you.

## WAL and concurrency

`PRAGMA journal_mode=WAL` is the recommended mode for any
long-running application.  It enables:
- Readers don't block writers, writers don't block readers.
- One writer at a time across processes (sqlite-level lock).

With `PRAGMA locking_mode=EXCLUSIVE`, no other process can open the
database while yours is running.  Use when you're sure you're the
sole user — it skips file-lock dance per transaction.

For multi-thread access from one process, either:
- Use `FULL_MUTEX` flag at open (let SQLite serialize), or
- Funnel all access through one thread (cbang's preferred pattern,
  driven by the event loop).

## Common pitfalls

- **Forgetting `init()` on NameValueTable.**  Prepared statements
  aren't built until `init()` runs.
- **Holding a Statement across schema changes.**  Drop and recompile
  after `ALTER TABLE`/`DROP TABLE`.
- **Long-lived transactions.**  An open `Transaction` blocks other
  writers.  Commit promptly.
- **Mixing `executef` with untrusted strings.**  It's printf, not
  parameter binding.  No SQL escaping.
- **Bind with a wrong type.**  `bind((int)x)` for a 64-bit param
  truncates; explicitly cast to `int64_t`.
- **`column()` lifetime.**  Don't keep a `Column` value past the
  next `next()` call.

## See also

- `cbang/db/Database.h`, `cbang/db/Statement.h`,
  `cbang/db/NameValueTable.h`, `cbang/db/Parameter.h`,
  `cbang/db/Column.h`, `cbang/db/Transaction.h`,
  `cbang/db/Backup.h`.
- [JSON.md](JSON.md) — for the `getJSON` / `readAll(sink)` JSON
  interop.
- [MariaDB.md](MariaDB.md) for the MariaDB / MySQL counterpart
  (event-driven async client).
- fah-client-bastet `App.cpp` (per-table cache via `getDB()`),
  `Unit.cpp` (`save()` to `units` table), `Account.cpp` (account
  tokens in `config` table).
