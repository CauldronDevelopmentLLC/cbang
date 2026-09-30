# cbang ClickHouse

`cb::ClickHouse::Client` talks to ClickHouse over its
[HTTP interface](https://clickhouse.com/docs/interfaces/http), on
`cb::HTTP::Client`, so it is fully asynchronous on your `cb::Event::Base` and
needs no third-party library.  It is always built.  Queries bind values as
ClickHouse server-side parameters and results come back as parsed JSON.

## Concepts

- **`cb::ClickHouse::Client`** — a configured endpoint: URL, user, password,
  default database and timeout.  `query()` and `insert()` each make one HTTP
  request and call back once with a `Response`.
- **`cb::ClickHouse::Response`** — the outcome: success, HTTP status,
  ClickHouse error code and message, and the parsed `JSONCompact` result.

## Client

```cpp
#include <cbang/db/clickhouse/Client.h>

cb::Event::Base base;
cb::ClickHouse::Client ch(new cb::HTTP::Client(base));
ch.addOptions(options);   // --clickhouse-url, --clickhouse-user,
                          // --clickhouse-pass, --clickhouse-reader-user,
                          // --clickhouse-reader-pass, --clickhouse-db,
                          // --clickhouse-timeout
```

| Option | Default | |
|---|---|---|
| `clickhouse-url` | `http://127.0.0.1:8123/` | HTTP interface URL; `https` needs an `SSLContext` on the `HTTP::Client` |
| `clickhouse-user` | `default` | Sent as `X-ClickHouse-User` |
| `clickhouse-pass` | | Sent as `X-ClickHouse-Key`, never in the URL |
| `clickhouse-reader-user` | | Login for read-only queries, see below |
| `clickhouse-reader-pass` | | Password for `clickhouse-reader-user` |
| `clickhouse-db` | | Default database, sent as `database=` |
| `clickhouse-timeout` | `30` | Seconds to wait for a response |

Each has a getter and setter, e.g. `setURL()`, `setTimeout()`.

`query(…, readOnly = true)` uses the reader login when
`clickhouse-reader-user` is set, and the main login otherwise.  Inserts
always use the main login.  Give the reader a read-only profile, so the
server itself refuses writes from queries rather than relying on the
`readonly` setting each query sends.

## Queries

```cpp
cb::JSON::ValuePtr params = new cb::JSON::Dict;
params->insert("since", "2026-09-01");
params->insert("type", "wu");

ch.query("SELECT type, count() AS n FROM grow.events "
         "WHERE ts >= {since:Date} AND type = {type:String} GROUP BY type",
         params, 0, [] (const cb::ClickHouse::Response &res) {
  if (res.isOk())
    for (auto &row: *res.getData())
      LOG_INFO(1, row->getString(0) << ": " << row->getU64(1));
  else LOG_ERROR(res.getMessage());
});
```

The SQL is the POST body.  Each `params` entry is sent as `param_<name>` and
fills ClickHouse's `{<name>:<Type>}` placeholder, so values are never spliced
into SQL.  `settings` is an optional dict of extra ClickHouse settings, sent
as URL parameters; one whose value is null is not sent.  Every request also
sends `default_format=JSONCompact`,
`output_format_json_quote_64bit_integers=0` (64-bit integers are JSON
numbers, which cbang's JSON parses exactly) and `wait_end_of_query=1`, so an
error sets the HTTP status instead of truncating a `200`.

### Parameter values

`Client::formatParam()` writes a JSON value in ClickHouse's text format:

| JSON | Parameter text |
|---|---|
| null | `\N` (NULL for a `Nullable(...)` type) |
| `true` / `false` | `true` / `false` |
| number | as cbang writes it in JSON |
| string | as is, except `\` → `\\`, tab → `\t`, newline → `\n` |
| list | `['a','b\'c',NULL,1]` |
| dict | `{'key':'value','n':1}` |

Inside a list or dict, strings are single-quoted and `'` is escaped as `\'`,
and null is `NULL`.  These rules were checked against ClickHouse 26.3: an
unescaped tab or newline makes a `String` parameter fail to parse, a lone
trailing `\` is an error, and an unescaped `\t` would become a tab.  Quotes
and UTF-8 need no escaping at the top level.  Values travel URL-encoded, and
cbang escapes `+`, which ClickHouse would read as a space.

### Results

`getJSON()` is the whole `JSONCompact` document; `getMeta()` and `getData()`
return its `meta` (column name and type) and `data` (rows as lists) lists,
empty for a statement which returns no result.

## Inserts

```cpp
cb::JSON::ValuePtr settings = new cb::JSON::Dict;
settings->insert("async_insert", 1);
settings->insert("wait_for_async_insert", 1);

ch.insert("grow.events", *rows, settings, cb);
```

`rows` is a list of objects, sent as `JSONEachRow` with
`query=INSERT INTO <table> FORMAT JSONEachRow`.  The table name is used as
given, so it must be trusted.

## Responses and errors

| Method | |
|---|---|
| `isOk()` | HTTP `200` and no error |
| `hasResponse()` | False when ClickHouse could not be reached or timed out |
| `isTimeout()` | The client timed out or ClickHouse raised `TIMEOUT_EXCEEDED` (159) |
| `isDataError()` | ClickHouse could not parse or accept the data, e.g. `CANNOT_PARSE_*`, `TYPE_MISMATCH`, `INCORRECT_DATA`, `VIOLATED_CONSTRAINT` |
| `getStatus()` | The HTTP status, zero without a response |
| `getCode()` | `X-ClickHouse-Exception-Code`, zero if none |
| `getError()` | The error message, without ClickHouse's `Code: N. DB::Exception:` prefix and version suffix |
| `getMessage()` | `ClickHouse:<code>: <error>` |

ClickHouse's HTTP status alone does not classify an error: a syntax error is
a `400`, an unknown table a `404`, bad credentials a `403`, a timeout a
`408`, but `READONLY` (164) and several data errors, e.g. an invalid `Bool`,
an over-long `FixedString` or a violated constraint, are a `500`.  Use the
code.

## Common pitfalls

- **DateTime parameters.**  A `{t:DateTime}` or `{t:DateTime64(3)}` value
  must be `YYYY-MM-DD hh:mm:ss[.fff]` or a Unix time; ISO 8601 with `T`,
  `Z` or an offset is rejected whatever `date_time_input_format` says.  Pass
  such times as a `String` and convert in SQL with
  `parseDateTime64BestEffort({t:String}, 3)`, or use a `Date` parameter.
- **`readonly`.**  A user whose profile sets `readonly` cannot be sent a
  lower value; `readonly=2` still allows other settings.
- **Floating point.**  Non-integer numbers are written like cbang's JSON,
  with at most six decimal places.  Pass exact decimal text as a string.
- **Blocking.**  None: everything runs on the event loop.  The timeout is the
  client's; set `max_execution_time` to have ClickHouse stop the query too.

## See also

- `cbang/db/clickhouse/Client.h`, `cbang/db/clickhouse/Response.h`.
- [API.md](API.md) — the `clickhouse:` and `clickhouse-insert:` endpoints.
- [WebServer.md](WebServer.md) — the HTTP client underneath.
- [MariaDB.md](MariaDB.md) — the other database client.
