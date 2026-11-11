# Upgrade notes

Changes that a downstream may have to act on, newest first.

## Sessions, OAuth2 logins and DB errors (2026-11)

These changes came from a security review.  Most need no downstream change.
The table says which ones might need one.

| Change | Default | Downstream action |
|---|---|---|
| OAuth2 `state` is no longer the session ID | always | None, unless you override `Provider::getRedirectURL()` without calling the base class |
| New session ID on login | **on** | Needed if your login SQL looks up data by the pre-login session ID, or if you set the session cookie yourself |
| Session IDs carry 256 random bits | always | None: still 43 URL Base64 characters |
| No session ID in login replies | **off** (ID omitted) | Needed only if a client reads `id` from the reply |
| Only `SIGNAL`ed DB errors reach clients | always | None, unless a client parses server DB error text |
| Origin check | off | Optional: turn it on |

### OAuth2 state is not the session ID

**Before:** the `state` sent to the OAuth2 provider was the session ID.  So the
session ID appeared in the provider's URL, in browser history, in access logs
and in `Referer` headers.

**Now:** `Provider::getRedirectURL(path, key)` sends `makeState(key)`.  That is
a 128-bit random nonce plus an HMAC-SHA256 of the nonce, keyed with `key`.
`Provider::isForgery(uri, key)` checks the state with `checkState(state, key)`.
The callers still pass the session ID as `key`, so their code is unchanged.
No server-side state is stored, so a callback served by another instance of
the server still works.  A wrong or missing state is still a forgery, and the
API login still replies `401` to it.

Action needed only if a downstream:

- overrides `getRedirectURL()` without calling `Provider::getRedirectURL()`
  and so sends its own state: make the state with `makeState(key)`.
- compares `uri.get("state")` to the session ID itself: call
  `checkState(state, sessionID)` instead.

`makeState()` and `checkState()` are virtual, so a downstream can replace them.

### A login gives the session a new ID

**Before:** an OAuth2 or `provider: none` login authenticated the session
under its existing ID.  An attacker who could plant a session ID in the victim
browser's cookie was logged in as the victim afterwards (session fixation).

**Now:** when a login succeeds, `API::Login` (and `OAuth2::LoginHandler`) calls
`SessionManager::rotateSession()`.  That function:

- moves the session, with all its data, to a new ID,
- closes the old ID,
- sets the new session cookie with `SessionManager::sendCookie()`,
- sets `Cache-Control: no-store` on the reply.

The login's SQL and its callback see the new ID as `{session.id}` and the old
one as `{session.previous_id}`.  The old ID is then removed from the session.

Option `session-rotate-on-login` (and `SessionManager::setRotateOnLogin()`)
is on by default.  Turning it off restores the old, unsafe behavior.

Action needed if a downstream:

1. **Keeps data under the pre-login session ID** and uses it in the login SQL,
   e.g. a guest account found by `{session.id}`.  Use
   `{~session.previous_id}` for the old ID.  It is NULL when no rotation took
   place, so `COALESCE({~session.previous_id}, {session.id})` works either
   way.  A session stored in the DB must end up under the new
   `{session.id}`.
2. **Sets the session cookie itself** with attributes other than cbang's (not
   HttpOnly, Secure, SameSite=None).  The rotated cookie uses the
   `SessionManager` attributes.  Set them to match your own cookie:
   `setCookieHTTPOnly()`, `setCookieSecure()`, `setCookieSameSite()`.
   Otherwise the cookie after login has different attributes, e.g. it loses
   `HttpOnly`, or is `Secure` on a plain-HTTP dev server, where the browser
   drops it and the login seems to fail.
3. **Has clients that hold the session ID themselves**, e.g. clients that send
   it in `Authorization` rather than use the cookie.  Their ID stops working
   at login.  Either get the new ID to them (see the next section) or set
   `session-rotate-on-login=false`.
4. **Logs users in with its own code**, outside `API::Login`.  Call
   `rotateSession()` and `sendCookie()` there too.  `Context::getAPI()` gives
   the API, and with it the session manager.

`SessionManager::generateID()` is now virtual, so a test can make IDs
deterministic.

### Session IDs

**Before:** an ID was the SHA-256 of the client address, the time and 64
random bits: 43 characters, but only 64 bits of entropy.

**Now:** 256 bits from `RAND_bytes()`, URL Base64: still 43 characters from
`[A-Za-z0-9_-]`.  Without OpenSSL, `std::random_device` replaces `rand()`.
Columns and cookie parsing that fit the old IDs fit the new ones.

### No session ID in login replies

**Before:** `GET /login/<provider>` replied `{id, redirect}`.  A completed
login without a `redirect`, and the login handler's who-am-I reply, sent the
whole session, `id` included.

**Now:** these replies leave out `id`.  The session ID is only in the cookie.
To put it back, for clients that read it, set option `session-login-reply-id`
(`SessionManager::setLoginReplyID(true)`).  This exposes the ID to scripts
and, for the `{id, redirect}` reply, to anything that logs response bodies.

No client on the maintainers' machines read `id`: not the JmpSaaS shell,
CAMotics Online, or the tests.  The FAH clients were not checked.

### DB errors

**Before:** a query endpoint's error reply was the server's full error text,
e.g. `{"error": "DB:1062: Duplicate entry 'bob' for key 'handle'"}`.

**Now:** a client gets the text only for errors that a procedure raised with
`SIGNAL` (`DB::isUserError()`: SQLSTATE class `45`, or error 1643 or 1644),
and only with a status below 500.  Any other error gets the status name,
e.g. `{"error": "HTTP_CONFLICT", "code": 409}`.  The status codes are as
before.  The full text is still logged, as a warning, or as an error for
500s.

Action needed only if a client parsed server error text, e.g. matched
`Duplicate entry`.  Have the procedure `SIGNAL` a message meant for users
instead.

### Origin check (opt-in)

`API::setOriginCheck(true)` refuses, with `403`, every WebSocket upgrade and
every request other than GET, HEAD or OPTIONS whose `Origin` header is neither
the request's `Host` nor an origin added with `API::addAllowedOrigin()`.  A
request without an `Origin`, e.g. from a non-browser client, is allowed.  It
is off by default.  When you turn it on:

- add your public origin if a proxy rewrites `Host`;
- add each origin that CORS lets make state-changing requests.

### New API, all compatible

- `HTTP::SessionManager`: `rotateSession()`, `sendCookie()`, cookie
  attribute setters, `get/setRotateOnLogin()`, `get/setLoginReplyID()`,
  options `session-rotate-on-login` and `session-login-reply-id`.
  `generateID()` is now virtual.
- `OAuth2::Provider`: `makeState()`, `checkState()`.
- `API::API`: `hasSessionManager()`, `setOriginCheck()`,
  `addAllowedOrigin()`, `isOriginAllowed()`.
- `API::Context::getAPI()`, `API::Login::getSessionReply()`.
- `MariaDB::DB::isUserError()`.

Some classes gained members: `SessionManager`, `API`, `API::Login`.
Rebuild everything that links cbang.
