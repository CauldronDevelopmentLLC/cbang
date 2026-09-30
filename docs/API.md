# cbang API Framework

`cb::API::API` is cbang's declarative REST/RPC framework.  You describe your
endpoints in a YAML or JSON config, optionally bind C++ callbacks to named
keys referenced from the config, and the framework routes requests, validates
arguments, dispatches DB queries, enforces access control, and writes JSON
responses — all event-driven on the cbang HTTP stack.

The config schema is the JmpAPI schema; the config must declare
`jmpapi: 1.2.0` (or later).

## Concepts

- **`cb::API::API`** — the registry.  Loads a config, holds bound callbacks
  and named queries, and is itself an `HTTP::RequestHandler` you can mount on
  an `HTTP::Server`.
- **Endpoint** — a URL pattern with per-method configs.
- **Statement** — a method body: a single handler config, an `if`/`then`/
  `else` conditional, or a list of statements run as a sequence.
- **Handler chain** — handlers take `(ctx, next)` where `next` is a
  continuation.  A handler replies, or calls `next(ctx)` to pass downstream —
  including from an async callback, so sequences and conditionals work across
  DB queries and subprocesses.
- **`cb::API::Context`** — bundles the request, parsed args, the resolver,
  and `reply(...)` shortcuts.  Held by `SmartPointer`; replies may come from
  any later callback.
- **Resolver** — interpolates `{refs}` (args, session, options, request body
  metadata) into SQL, headers, exec input, and conditions.

## Minimal example

```cpp
#include <cbang/api/API.h>
#include <cbang/http/Server.h>
#include <cbang/json/YAMLReader.h>

cb::Event::Base  base;
cb::HTTP::Server http(base);
cb::API::API     api(options);

api.bind("hello", [] (cb::JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("msg", "world");
  sink.endDict();
});

api.setDBConnector(new cb::MariaDB::Connector(base));
api.load(cb::JSON::YAMLReader::parseFile("api.yaml"));

http.addHandler(&api);
```

And `api.yaml`:

```yaml
jmpapi: 1.2.0
info: {title: My API, version: 1.0.0}

endpoints:
  /hello:
    get: {bind: hello}

  /users/{id}:
    args: {id: {type: u32}}
    get: {sql: "CALL UserGet({args.id})", return: dict}
```

## Config structure

Top-level keys:

| Key | Meaning |
|---|---|
| `jmpapi` | Schema version, at least `1.2.0`.  Required. |
| `info` | OpenAPI info block (title, version, ...). |
| `endpoints` | URL pattern → endpoint config. |
| `args` | Named, reusable arg declarations. |
| `queries` | Named, reusable SQL queries. |
| `apis` | Optional category → `{args, queries, endpoints}` grouping; categories become OpenAPI tags. |

URL patterns capture path segments with `{name}`, e.g.
`/users/{id}/posts/{slug}`; a capture becomes an arg only once it is also
declared in an `args:` block (see below).  Patterns nest: a key starting
with `/` inside an endpoint config is a sub-pattern.

Method keys (`get`, `put`, `post`, `delete`, ..., `any`, or a combination
like `get|put`) hold the method's statement.  Validation keys (`args`,
`allow`/`deny`, `body`/`files`) may sit at the endpoint or method level;
children inherit them.

## Endpoint types

The endpoint type is given by the `handler` key, or inferred: `bind` →
bound callback, `sql`/`query` → DB query, `clickhouse`, `clickhouse-insert`,
`timeseries`, `path` → file, `resource`, else `pass`.

| Handler | Purpose |
|---|---|
| `bind` | Dispatch to a C++ callback registered with `api.bind(key, ...)`. |
| `query` (or `sql`) | Run SQL and stream the result as JSON.  See below. |
| `clickhouse` | Run a ClickHouse query and reply with its result.  See below. |
| `clickhouse-insert` | Insert the JSON rows of the request body into ClickHouse. |
| `status` | Fixed status reply (`code`, `text`). |
| `redirect` | Redirect (`location`, `code`). |
| `cors` | CORS headers / preflight (`origins`, `methods`, ...). |
| `file` / `resource` | Serve from disk / compiled-in resources. |
| `spec` | Serve the generated OpenAPI spec. |
| `websocket` | Upgrade and route websocket messages. |
| `login` / `logout` | OAuth2 session login flow (with the session/OAuth2 subsystems injected). |
| `timeseries` | LevelDB-backed timeseries query/subscribe. |
| `pass` | Do nothing; pass to the next handler. |

A `handlers` list runs several handler configs in order.

## Statements, sequences and pre-steps

A method body is a *statement*: a handler config, a conditional, or a YAML
list of statements folded into a chain (each statement either replies or
passes to the next):

```yaml
/resize/{n}:
  put:
    - exec: {cmd: '{options.scripts}/check.py'}   # may reply or pass
    - {sql: "CALL Resize({args.n})"}
```

Within a statement, `headers:` (response headers) and `exec:` run as
pre-steps before the endpoint handler.

### Exec

`exec` runs an external program through the subprocess pool
(`api.setProcPool(...)`).  Its `input` envelope is resolved and written to
the program's stdin as JSON; the program's JSON reply can merge `args`, set
`headers`, and either continue the chain or reply directly.

```yaml
get:
  exec:
    cmd: '{options.scripts}/transform.py'
    input: {n: '{args.n}', label: 'x-{args.n}'}
```

### Conditions

`if`/`then`/`else` selects a statement at request time.  Evaluation is
continuation-based, so async conditions compose with `and`/`or`
short-circuiting:

```yaml
get:
  if:   {'<': ['{args.n}', 100]}
  then: {handler: status, code: 200, text: small}
  else: {handler: status, code: 200, text: big}
```

| Condition | True when |
|---|---|
| `exists: <path>` | The file path (after interpolation) exists. |
| `=` / `!=` / `<` / `<=` | JSON comparison of the two operands. |
| `not` | The sub-condition is false. |
| `and` / `or` | All / any of the sub-conditions (short-circuit). |
| `sql` | The query returns a truthy single value (async). |
| `cmd` | The command exits 0 (async, subprocess pool). |

## Variables and typed values

`{ref}` resolves from the resolver namespaces: `{args.*}`, `{session.*}`,
`{group}`, `{options.*}`, `{request.*}` (`method`, `host`, `path`, `ip`,
and `headers.<name>`), `{info.*}` (application, build and system metadata
as `{info.<category>.<key>}`, e.g. `{info.Build.Version}`), plus the binary
roots below.  `{args.*}` holds exactly the args declared by the `args:`
blocks in scope — anything undeclared, including an undeclared URL capture,
is dropped, and a method with no `args:` anywhere in its chain has no
`args` root at all.  In SQL every ref is
bound as a prepared-statement parameter (never spliced into the SQL text), in
ClickHouse SQL as a typed server-side parameter; elsewhere refs interpolate
as strings.  A missing ref is a request-time error; a `{~ref}` resolves null
(SQL `NULL`) when missing.  A config value that is a lone `'{ref}'` resolves
to the referenced *native* JSON value (number, bool, ...), not a string —
which is what makes numeric compares and typed `exec` input work.
`{options.*}` refs resolve into the config once at load time and so may form
SQL text.  `{{` and `}}` write literal braces.

## Queries

`sql` (or `query: <name>` for one defined under `queries:`) runs through the
injected `MariaDB::Connector` as a prepared statement.  `return` selects the
result shape:

| `return` | Response |
|---|---|
| `ok` | `200` with no body content (default). |
| `dict` | First row as an object; no row → `404`. |
| `list` | Rows as a list (single column → scalars, else objects). |
| `hlist` | Header row of column names, then row lists. |
| `fields` | Multiple result sets mapped by the `fields` list. |
| `one` | Single value, row 1 / column 1; no row → `404`. |
| `bool` / `u64` / `s64` | Single value, coerced. |
| `binary` | Raw response body.  See below. |

## ClickHouse queries

`clickhouse` runs SQL through the injected `ClickHouse::Client` (see
[ClickHouse.md](ClickHouse.md)):

```yaml
/stats/events:
  get:
    args:
      since: {type: time}
      type:  {type: string, optional: true}
    clickhouse: >-
      SELECT toDate(ts) AS day, type, count() AS events
      FROM grow.events
      WHERE ts >= parseDateTime64BestEffort({args.since:String}, 3)
        AND ({~args.type:Nullable(String)} IS NULL
             OR type = {~args.type:Nullable(String)})
      GROUP BY day, type ORDER BY day, type
    return: list
    settings: {max_execution_time: 10}
```

Every `{ref}` carries a ClickHouse type after a colon, e.g.
`{args.id:UInt32}`.  At request time the refs become server-side parameters
`{p0:UInt32}`, `{p1:...}`, numbered in order of appearance, and their values
are sent separately, so they are never spliced into the SQL.  A ref without a
type, or a binary ref, is a request-time error.  A `{~ref:Type}` binds NULL
when missing, so its type should be `Nullable(...)`.  `{options.*}` refs take
no type; as with `sql`, they are spliced into the SQL at load time.  A list
value binds as an array and a dict as a map.

| Key | Meaning |
|---|---|
| `clickhouse` | The SQL. |
| `return` | As for `sql`: `ok` (default), `pass`, `list`, `hlist`, `dict`, `one`, `bool`, `u64` or `s64`. |
| `into` | Capture the result as this var and continue, as for `sql`. |
| `settings` | Extra ClickHouse settings, resolved per request.  One which resolves null is not sent. |
| `readonly` | Default `true`, which sends `readonly=2`.  `false` allows writes. |

A query only reads unless `readonly: false`; writes should normally go
through `clickhouse-insert`.  Read-only queries use the reader login,
`clickhouse-reader-user`, when one is configured, so the server can enforce
it too; `readonly: false` queries and inserts use `clickhouse-user`.  A ClickHouse error replies `500` with
`{"error": "ClickHouse:<code>: <message>", "code": 500}` and is logged.  A
timeout, ClickHouse's (code 159) or the client's, replies `504`, and failing
to reach ClickHouse `503`.

## ClickHouse inserts

`clickhouse-insert` appends the rows of a JSON request body, one row object
or a list of them, to a table with a ClickHouse async insert:

```yaml
/events:
  post:
    allow: [$ingest]
    body: {required: true, max-size: 16MB, type: application/json}
    clickhouse-insert:
      table: grow.events
      row:
        type:    {type: string, max: 64}
        machine: {type: string, optional: true}
      set:
        source: as
        ingested_by: '{session.user}'
      max-rows: 10000
```

| Key | Meaning |
|---|---|
| `table` | Required `[db.]table`, checked at load. |
| `row` | Arg declarations, as for `args`, validating each row.  A field neither declared nor in `set` is rejected. |
| `set` | Fields set in every row, overwriting the client's, resolved per request.  A lone `'{ref}'` keeps its native type. |
| `max-rows` | Default `10000`. |
| `wait` | Default `true`: reply once ClickHouse has written the rows. |
| `settings` | Extra ClickHouse settings, resolved per request.  One which resolves null is not sent. |

The insert sends `async_insert=1`, `wait_for_async_insert` from `wait`,
`date_time_input_format=best_effort`, `input_format_skip_unknown_fields=0`
and `input_format_null_as_default=1`, then `settings`, and replies
`{"rows": N}`.  Nothing is buffered in the API: durability comes from
ClickHouse acknowledging the insert and from clients retrying failures.

| Failure | Reply |
|---|---|
| The body is not JSON, not objects, or has trailing data | `400` |
| A row fails `row` validation or has an unknown field | `400`, naming the row index and field |
| More than `max-rows` rows | `413` |
| ClickHouse rejects the data: a parse, type or constraint error code | `400`, with ClickHouse's message |
| ClickHouse cannot be reached or times out | `503`, for the client to retry |
| Any other ClickHouse error | `500` |

## Binary data

JSON has no byte type, so binary rides outside the args dict.  A raw request
body is exposed as `{body}` (`.size`, `.type`); `multipart/form-data` file
parts as `{files.<name>}` (`.filename`, `.type`, `.size`); plain multipart
fields fold into `{args.*}`.

A binary ref used in SQL binds its bytes, exactly like any other ref, so
blobs of any content and size are safe.  Metadata refs resolve normally.
Like any ref, the bytes cannot sit inside a string literal — assemble
strings in SQL with `CONCAT()`.

```yaml
/avatar:
  put:
    body: {required: true, max-size: 1MB, type: image/*}
    sql:  "CALL SetAvatar({session.user}, {body}, {body.type})"

/avatar/{id}:
  get:
    args: {id: {type: u32}}
    sql:  "CALL GetAvatar({args.id})"
    return: binary
```

`return: binary` replies with row 1 / column 1 as the raw body.  The
`Content-Type` comes from the `content-type` key (interpolated), else a
second selected column, else `application/octet-stream`; no row → `404`.

`body:` and `files:` declaration blocks validate before the statement runs:
`required` → `400`, `max-size` (`5MB`, `512K`, ...) → `413`, `type` (a media
type, `*` glob, or list) → `415`.  Declarations also appear in the OpenAPI
spec as the request body.

## Bearer tokens

Machine clients may send `Authorization: Bearer <JWT>` instead of a session
ID.  Install an `HTTP::BearerSessionManager` with the public keys that sign
the tokens, and the usual `session` handler does the rest:

```cpp
auto sessions = SmartPtr(new cb::HTTP::BearerSessionManager);
sessions->readPublicKeys("bearer.pub"); // One or more PEM public keys
api.setSessionManager(sessions);
```

A token signed with RS256 by one of the keys gets a session whose ID,
`{session.id}`, is the token's SHA-256, whose user, `{session.user}`, is its
`sub` claim and whose groups are its `groups` claim.  It is not in
`authenticated`, so `allow: [$authenticated]` does not admit a token; allow
its groups instead.

- The token is verified locally, never looked up in the DB, and the session
  is cached until its `exp` claim, if any.
- An invalid, expired (`exp`) or not yet valid (`nbf`) token, or one signed
  otherwise, e.g. `"alg": "none"`, replies `401`.
- A token is revoked only by rotating the signing key.  Keys may overlap
  during a rotation: list both until tokens signed with the new key are in
  use.

## Bound C++ callbacks

Three signatures, picked by the `bind` overload:

```cpp
api.bind("ping", [] (cb::JSON::Sink &sink) {...});      // write JSON, 200
api.bind("get",  [] (const cb::API::CtxPtr &ctx) {      // full control
  uint64_t id = ctx->getArgs()->getU64("id");
  ctx->reply(...);                                       // now or later
});
```

`Context` shortcuts:

```cpp
ctx->reply(json)                                 // JSON value
ctx->reply(code, "message")                      // status + text
ctx->reply([&] (cb::JSON::Sink &sink) {...})     // streamed JSON
ctx->getArgs()                                   // merged, validated args
ctx->getRequest()                                // HTTP::Request &
ctx->getWebsocket()                              // WebsocketPtr (or null)
```

Replying with a `cb::API::Blob` sends a raw binary response.

## Subsystem injection

All optional; set the ones your config uses, before `load()`:

```cpp
api.setDBConnector(connector);        // sql / query endpoints
api.setProcPool(procPool);            // exec and cmd conditions
api.setSessionManager(sessions);      // sessions, allow/deny groups, and
                                      // bearer tokens with a
                                      // BearerSessionManager
api.setOAuth2Providers(providers);    // login endpoints
api.setClient(httpClient);            // OAuth2 HTTP client
api.setTimeseriesDB(levelDB);         // timeseries endpoints
api.setClickHouse(clickHouse);        // clickhouse / clickhouse-insert
```

## OpenAPI spec

`load()` builds an OpenAPI 3.1 spec from the config: paths, parameters from
`args`, request bodies from `body`/`files`, tags from `apis` categories,
descriptions from `help`.  `hide: true` omits an endpoint.  Serve it with:

```yaml
/openapi-spec:
  get: {handler: spec}
```

## Common pitfalls

- **Bind name mismatch.**  A config referencing `bind: foo` with no
  `api.bind("foo", ...)` fails at `load()`.  Bind first.
- **Missing subsystem.**  `sql:` without `setDBConnector`, `exec:`
  without `setProcPool`, or `clickhouse:` without `setClickHouse`, fails at
  `load()`.
- **Replying twice.**  Each Context maps to one HTTP response.
- **Returning without replying or passing.**  An async handler that drops
  both `ctx` and `next` leaves the request hanging.
- **Binary refs in string context.**  `'x-{body}'` is an error anywhere;
  `{body}` is only valid bound whole into SQL or as the response body.
- **Undeclared arg.**  `{args.x}` where `x` is a URL capture but has no
  `args:` entry is a request-time error, not a silent empty value.  The
  OpenAPI spec still lists such captures as path parameters, so it is not a
  check on this.
- **Optional arg without `~`.**  An `optional: true` arg is absent when not
  supplied, so it must be referenced `{~args.x}`.  An arg with a `default:`
  is always present and uses `{args.x}`.
- **ClickHouse DateTime parameters.**  `{args.t:DateTime64(3)}` only parses
  `YYYY-MM-DD hh:mm:ss[.fff]` or a Unix time, not ISO 8601 with `T`, `Z` or
  an offset, which a `type: time` arg is.  Use
  `parseDateTime64BestEffort({args.t:String}, 3)` or a `Date` parameter.
- **Literal braces in SQL.**  A regex `'\d{4}'` or JSON `'{}'` in `sql:` or
  `clickhouse:` must be written `'\d{{4}}'`, `'{{}}'`.
- **String length.**  A `string` arg's length limits are `min` and `max`;
  unknown keys such as `max-length` are ignored.

## See also

- `cbang/api/API.h`, `cbang/api/Context.h`, `cbang/api/Handler.h`,
  `cbang/api/Resolver.h`, `cbang/api/QueryDef.h`, `cbang/api/Blob.h`,
  `cbang/api/condition/`, `cbang/api/handler/`.
- [WebServer.md](WebServer.md) — the underlying HTTP layer.
- [JSON.md](JSON.md) — sink-based response writing.
- [MariaDB.md](MariaDB.md) — the DB layer behind queries.
- [ClickHouse.md](ClickHouse.md) — the client behind ClickHouse endpoints.
