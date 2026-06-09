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
`/users/{id}/posts/{slug}`; captures become args.  Patterns nest: a key
starting with `/` inside an endpoint config is a sub-pattern.

Method keys (`get`, `put`, `post`, `delete`, ..., `any`, or a combination
like `get|put`) hold the method's statement.  Validation keys (`args`,
`allow`/`deny`, `body`/`files`) may sit at the endpoint or method level;
children inherit them.

## Endpoint types

The endpoint type is given by the `handler` key, or inferred: `bind` →
bound callback, `sql`/`query` → DB query, `timeseries`, `path` → file,
`resource`, else `pass`.

| Handler | Purpose |
|---|---|
| `bind` | Dispatch to a C++ callback registered with `api.bind(key, ...)`. |
| `query` (or `sql`) | Run SQL and stream the result as JSON.  See below. |
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

## Interpolation and typed values

`{ref}` interpolates from the resolver namespaces: `{args.*}`,
`{session.*}`, `{group}`, `{options.*}`, plus the binary roots below.  In
SQL, strings are quoted and escaped; numbers and booleans are not.  A config
value that is a lone `'{ref}'` resolves to the referenced *native* JSON value
(number, bool, ...), not a string — which is what makes numeric compares and
typed `exec` input work.

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

## Binary data

JSON has no byte type, so binary rides outside the args dict.  A raw request
body is exposed as `{body}` (`.size`, `.type`); `multipart/form-data` file
parts as `{files.<name>}` (`.filename`, `.type`, `.size`); plain multipart
fields fold into `{args.*}`.

A binary ref used in SQL is **bound as a real prepared-statement parameter**
(the resolver emits `?` and passes the bytes through), so blobs of any
content and size are safe.  Metadata refs interpolate normally.  Using the
bytes inside a larger string is an error.

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
api.setSessionManager(sessions);      // sessions, allow/deny groups
api.setOAuth2Providers(providers);    // login endpoints
api.setClient(httpClient);            // OAuth2 HTTP client
api.setTimeseriesDB(levelDB);         // timeseries endpoints
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
- **Missing subsystem.**  `sql:` without `setDBConnector`, or `exec:`
  without `setProcPool`, fails at `load()`.
- **Replying twice.**  Each Context maps to one HTTP response.
- **Returning without replying or passing.**  An async handler that drops
  both `ctx` and `next` leaves the request hanging.
- **Binary refs in string context.**  `'x-{body}'` is an error anywhere;
  `{body}` is only valid bound whole into SQL or as the response body.

## See also

- `cbang/api/API.h`, `cbang/api/Context.h`, `cbang/api/Handler.h`,
  `cbang/api/Resolver.h`, `cbang/api/QueryDef.h`, `cbang/api/Blob.h`,
  `cbang/api/condition/`, `cbang/api/handler/`.
- [WebServer.md](WebServer.md) — the underlying HTTP layer.
- [JSON.md](JSON.md) — sink-based response writing.
- [MariaDB.md](MariaDB.md) — the DB layer behind queries.
