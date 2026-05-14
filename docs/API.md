# cbang API Framework

`cb::API::API` is cbang's declarative REST/RPC framework.  You describe
your endpoints in a JSON spec, bind handler callbacks to named keys
referenced from the spec, and the framework routes requests, parses
arguments, dispatches DB queries, enforces auth, and writes JSON
responses — all event-driven on the cbang HTTP stack.

It's the natural choice for cbang-based services that expose more than
a handful of endpoints.

## Concepts

- **`cb::API::API`** — the registry.  Loads a JSON spec, holds
  handler callbacks, exposes itself as an `HTTP::RequestHandler` you
  can mount on your `HTTP::Server`.
- **Endpoint** — one URL pattern + HTTP method + handler.  Defined
  declaratively in the spec.
- **Spec** — a JSON document mapping URL patterns to endpoint
  configs.  Endpoint configs can reference named callbacks, named
  queries (DB-backed), timeseries (LevelDB-backed), or built-in
  handlers (login, logout, session, redirect, etc.).
- **Callback** — your C++ code that implements an endpoint.  Three
  signatures, from simplest to richest: sink-callback (just write a
  JSON response), void-callback (the framework auto-replies), and
  full callback (you decide when/how to reply).
- **`cb::API::Context`** — passed to your callbacks.  Bundles the
  request, websocket (if upgraded), args (parsed from the URL +
  query + body), and a `reply(...)` shortcut.
- **`cb::API::QueryDef`** — declarative SQL query bound to a name.
  Resolves DB connection, parameterizes from args, streams results
  to a JSON sink.
- **Built-in helpers** — sessions, OAuth2 login, MariaDB pool,
  subprocess pool, timeseries DB are wired in if you provide them.

## Minimal example

```cpp
#include <cbang/api/API.h>
#include <cbang/api/Context.h>
#include <cbang/http/Server.h>

class MyService {
  cb::Event::Base  base;
  cb::HTTP::Server http{base};
  cb::API::API     api;

public:
  MyService(cb::Options &opts) : api(opts) {
    // Bind C++ callbacks to names referenced in the spec.
    api.bind("hello", [] (cb::JSON::Sink &sink) {
      sink.beginDict();
      sink.insert("msg", "world");
      sink.endDict();
    });

    api.bind("echo", [] (const cb::API::CtxPtr &ctx) {
      ctx->reply(ctx->getArgs());          // echo args back
    });

    // Load the spec.
    api.load(cb::JSON::Reader::parseFile("api.json"));

    // Mount on the HTTP server.
    http.addHandler("/api/.*", &api);
  }

  void run() {
    http.init(api.getOptions());
    base.dispatch();
  }
};
```

And `api.json`:

```json
{
  "/hello":            { "handler": "hello" },
  "/echo":             { "handler": "echo", "methods": "POST" },
  "/items/<id:uint>":  { "query": "get-item" }
}
```

The framework will:
- Match incoming requests against URL patterns
- Resolve arg types (`<id:uint>` becomes an integer arg named `id`)
- For named-handler endpoints, dispatch to your bound callback
- For named-query endpoints, run the SQL and stream rows to the
  response
- Auto-emit `application/json` responses with appropriate status
  codes

## Spec format

The spec is a JSON map of URL pattern → endpoint config.  Patterns
use `<name>` for path variables, optionally typed with `:type`:

```
/users/<id:uint>/posts/<slug>
```

Recognised types include `uint`, `int`, `bool`, `string`,
`uuid`, `path` (matches `/`).

Endpoint config fields you'll commonly use:

| Field | Meaning |
|---|---|
| `handler` | Name of a bound callback (see `bind`). |
| `query` | Name of a `QueryDef` registered via `addQuery`. |
| `timeseries` | Name of a timeseries handler. |
| `methods` | HTTP method or array (`"GET"`, `["GET","POST"]`). |
| `args` | Named arg-spec reference; constrains/validates body/query args. |
| `auth` | Required permission level (used with sessions). |
| `category` | Group endpoints in generated specs/docs. |
| `redirect` | Redirect response. |
| `description` | Free-text for generated docs. |

Endpoint configs can nest:

```json
{
  "/api/v1": {
    "category": "v1",
    "/users":       { "query": "list-users" },
    "/users/<id:uint>": {
      "GET":    { "query": "get-user" },
      "PATCH":  { "handler": "update-user" },
      "DELETE": { "handler": "delete-user" }
    }
  }
}
```

Method-keyed sub-objects route by HTTP verb.

## Callbacks

Three signatures, picked by which overload of `bind` you use.

### Sink callback — simplest

For "always return this JSON":

```cpp
api.bind("ping", [] (cb::JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("ok", true);
  sink.endDict();
});
```

The framework wraps it in a 200 response.  No access to the
request, args, or session.

### Void callback — most common

For "do something with args; return JSON or an error":

```cpp
api.bind("get-item", [&] (const cb::API::CtxPtr &ctx) {
  auto args = ctx->getArgs();
  uint64_t id = args->getU64("id");

  if (auto item = store.find(id))
    ctx->reply(item->toJSON());
  else
    ctx->reply(cb::HTTP::HTTP_NOT_FOUND, "no such item");
});
```

`ctx->getArgs()` returns a `JSON::ValuePtr` with path/query/body
arguments merged.

### Full callback — most control

```cpp
api.bind("upload", [&] (const cb::API::CtxPtr &ctx) {
  auto &req = ctx->getRequest();
  req.getInputStream()->...;        // pull body manually
  // ... long-running work, async ...
  // call ctx->reply(...) eventually
});
```

Use when you need streaming, custom headers, deferred replies, or
WebSocket upgrades.

## Context shortcuts

```cpp
ctx->reply(200, "OK")                            // plain text
ctx->reply(json)                                 // JSON value
ctx->reply([&] (cb::JSON::Sink &sink) { ... })   // streamed JSON
ctx->reply(exception)                            // error mapping
ctx->getArgs()                                   // merged args
ctx->getRequest()                                // HTTP::Request &
ctx->getWebsocket()                              // WebsocketPtr (or null)
ctx->setSession(session)                         // attach a session
```

The `reply(exception)` overload picks an HTTP status from cbang's
known exception hierarchy and includes the message.

## Queries (`QueryDef`)

For database-backed endpoints — point a URL at a SQL query, let
the framework do the rest:

```cpp
class GetUser : public cb::API::QueryDef {
public:
  cb::SmartPointer<cb::MariaDB::EventDB> getDBConnection() const override
  { return /* pool->get() */; }
  // ... define query and arg shape ...
};

api.addQuery("get-user", new GetUser);
```

In the spec:

```json
{
  "/users/<id:uint>": { "query": "get-user" }
}
```

The framework binds the path's `id` to the SQL's `${id}`
placeholder, runs the query async, and streams the result rows
back as JSON.  See `cbang/api/QueryDef.h`, `cbang/api/Query.h`,
`cbang/api/SessionQuery.h`.

## Sessions, OAuth2, MariaDB, Subprocess pool

Wire up the supporting subsystems by injection — `API` takes a
SmartPointer for each.  All optional; only set the ones you use.

```cpp
api.setClient(httpClient);
api.setOAuth2Providers(oauth2Providers);
api.setSessionManager(sessionManager);
api.setDBConnector(mariaDBConnector);
api.setProcPool(procPool);
api.setTimeseriesDB(levelDB);
```

Once set, endpoint configs in the spec can reference auth, login,
queries, timeseries, etc., and the framework wires them through.

OAuth2 endpoints are typically:

```json
{
  "/auth/google":           { "login":  "google" },
  "/auth/google/callback":  { "login":  "google" },
  "/logout":                { "logout": true }
}
```

The Login handler uses the configured `OAuth2::Providers` to do the
flow.

## Mount on HTTP server

`cb::API::API` is itself an `HTTP::RequestHandler`.  Mount it under
a URL prefix:

```cpp
http.addHandler("/api/.*", &api);
```

Or expose it under multiple prefixes / aliases as needed.  The API
internally strips the prefix before matching against its spec.

## OpenAPI / spec export

The framework keeps a `spec` JSON that mirrors the loaded
configuration.  Expose it as a read-only endpoint for clients and
documentation:

```cpp
api.bind("api-spec", [&] (cb::JSON::Sink &sink) {
  api.getSpec()->write(sink);
});
```

```json
{ "/api-spec": { "handler": "api-spec" } }
```

## Patterns

### Validate args before handling

Define a named arg-spec and reference it:

```json
{
  "args": {
    "create-user": {
      "name":  { "type": "string", "required": true },
      "email": { "type": "string", "pattern": "^[^@]+@[^@]+$" }
    }
  },
  "/users": {
    "POST": { "handler": "create-user", "args": "create-user" }
  }
}
```

The framework rejects requests with bad args before the handler
sees them.

### Long-running endpoint

```cpp
api.bind("compute", [&] (const cb::API::CtxPtr &ctx) {
  procPool.run(WeakCall(this, [ctx] (auto &result) {
    ctx->reply(result.toJSON());      // fires when subprocess finishes
  }));
  // Don't reply yet — we'll reply from the subprocess callback.
});
```

`Context` is held by `SmartPointer`; the reply can come from any
later callback.

### Session-protected endpoint

```json
{
  "/me": {
    "auth":    "user",
    "query":   "current-user"
  }
}
```

With a `SessionManager` configured, the framework verifies the
session cookie and rejects unauthenticated requests automatically.

### Streaming a large response

```cpp
api.bind("export", [&] (const cb::API::CtxPtr &ctx) {
  ctx->reply([&] (cb::JSON::Sink &sink) {
    sink.beginList();
    for (auto &row: rows) {
      sink.beginAppend();
      sink.beginDict();
      sink.insert("id",   row.id);
      sink.insert("name", row.name);
      sink.endDict();
    }
    sink.endList();
  });
});
```

The sink writes directly to the response socket — no
intermediate buffer.

## Common pitfalls

- **Bind name mismatch.**  A spec referencing `"handler": "foo"`
  with no `api.bind("foo", ...)` results in a runtime 500 on the
  first request.  Bind before calling `api.load`.
- **Forgetting `setDBConnector`** when the spec uses
  `"query": "..."`.  Same kind of failure.
- **Replying twice.**  Each Context maps to one HTTP response.  Two
  `reply()` calls are a bug.
- **Returning instead of replying.**  Async endpoints that return
  without calling `reply` leave the request hanging.  Track this
  explicitly with WeakCall-bound callbacks.
- **Calling DB methods synchronously inside a handler.**  Use the
  async MariaDB API (or QueryDef machinery).  Blocking the loop
  blocks every other endpoint.
- **No spec at all.**  Without `api.load(...)`, the API object
  routes nothing.

## See also

- `cbang/api/API.h`, `cbang/api/Context.h`,
  `cbang/api/Handler.h`, `cbang/api/QueryDef.h`,
  `cbang/api/JSONResponse.h`, `cbang/api/Login.h`,
  `cbang/api/Websocket.h`, `cbang/api/Timeseries.h`.
- [WebServer.md](WebServer.md) — the underlying HTTP layer.
- [JSON.md](JSON.md) — for the sink-based response writing.
- [MariaDB.md](MariaDB.md) — for the DB backing `QueryDef`.
