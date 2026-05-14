# cbang ACMEv2 — Automated TLS Certificate Acquisition

cbang ships an ACMEv2 client (`cb::ACMEv2::Account`) that obtains and
renews TLS certificates from Let's Encrypt or any RFC 8555-compatible
CA, using HTTP-01 challenges served from your own HTTP server.  The
event loop drives the full state machine; you only register the
domains and hand it a key.

## What it does

The `Account` object:

1. Authenticates with the ACME directory using a JWK (your account
   key).
2. Submits orders for one or more sets of domains, each represented
   by a `KeyCert`.
3. Handles HTTP-01 challenges by serving the validation token from
   an HTTP route on your existing `cb::HTTP::Server`.
4. Polls the order, downloads the issued certificate chain, and
   invokes your listener callback.
5. Re-runs renewal automatically before each cert expires.

Everything is async: no thread, no blocking calls, full integration
with `cb::Event::Base`.

## Concepts

- **`cb::ACMEv2::Account`** — one ACME identity, one account key,
  one set of `KeyCert`s.  Uses a `cb::HTTP::Client` for outbound
  ACME calls.
- **`cb::ACMEv2::KeyCert`** — one cert + chain + domain list.  An
  Account manages a list of these; each renews independently.
- **Listener** — callback invoked when a `KeyCert` is updated (cert
  obtained or renewed).  Used to plumb the new chain into your
  `cb::SSLContext`.
- **HTTP-01 challenge** — the ACME server posts a challenge token
  to the URL `http://<domain>/.well-known/acme-challenge/<token>`.
  Your server must serve `<token>.<thumbprint>` from that path.
  `Account::challengeRequest` is the handler.

## Setup

You need:
- A live HTTP server (`cb::HTTP::Server`) reachable from the
  internet on port 80 for the challenge.
- An RSA account key (kept private, persisted across runs).
- One or more domains controlled by the host running this code.
- A list of contact emails for the CA (typically your operations
  address).

```cpp
#include <cbang/acmev2/Account.h>
#include <cbang/acmev2/KeyCert.h>

cb::Event::Base    base;
cb::HTTP::Client   client(base, new cb::SSLContext);
cb::HTTP::Server   httpServer(base);          // listens on :80
cb::ACMEv2::Account acme(client);

// Account key — load from disk or generate once and persist.
cb::KeyPair key;
key.generateRSA(2048);
acme.setKey(key);

// Direct against staging while testing.
acme.setURIBase(cb::ACMEv2::letsencrypt_staging);

// Contact info given to the CA.
acme.setContactEmails("ops@example.com");

// Per-domain cert holder.
cb::KeyPair certKey;
certKey.generateRSA(2048);
auto kc = cb::SmartPtr(new cb::ACMEv2::KeyCert("example.com", certKey));
kc->addDomain("www.example.com");
acme.add(kc);
```

When testing, use the staging directory
(`letsencrypt_staging`).  Switch to `letsencrypt_base` for
production.  Rate limits on production are real; staging is forgiving.

## Wiring the HTTP-01 challenge

Add a single route to your HTTP server that delegates to
`Account::challengeRequest`:

```cpp
httpServer.addMember(cb::HTTP::HTTP_GET,
  "/\\.well-known/acme-challenge/.*",
  &acme, &cb::ACMEv2::Account::challengeRequest);
```

`challengeRequest(req)` returns `true` and serves the challenge body
when the URL matches an active challenge; otherwise `false` so the
next handler can run.

## Renewal listener

```cpp
acme.addListener([&] (cb::ACMEv2::KeyCert &kc) {
  // New cert chain is in kc.getChain().  Install it on your TLS context.
  sslCtx->useCertificateChain(kc.getChain());
  sslCtx->usePrivateKey(kc.getKey());

  // Persist to disk too.
  kc.getChain().writeFile("cert.pem");
});
```

The listener fires:
- Once shortly after `acme.update()` if a new cert is issued.
- Each subsequent renewal cycle (~30 days before expiry by default).

## Driving the state machine

```cpp
acme.update();         // start now (or trigger an out-of-cycle renewal)
base.dispatch();
```

`update()` queues an immediate state-machine pass.  The Account
maintains an internal timer (`renewPeriod`, default 15 days) that
re-fires `update()` periodically to check expiry.

The state machine:

```
IDLE → GET_DIR → REGISTER → (for each KeyCert)
                              NEW_ORDER →
                                (for each authorization)
                                  GET_AUTH → CHALLENGE →
                              FINALIZE → GET_ORDER → GET_CERT →
                          ...listener fires...
                          → IDLE (next renewal)
```

On any failure, the state resets and retries after `retryWait`
(default 1200 s).  You can tune both:

```cpp
acme.setRetryWait(60 * 5);     // retry every 5 min on failure
acme.setRenewPeriod(30);       // check renewal every 30 days
```

## Configuration options

`Account::addOptions(options)` registers a set of CLI options on
your `cb::Application`:

| Option | Meaning |
|---|---|
| `acme-directory` | URI of the ACME directory (default Let's Encrypt staging). |
| `acme-emails` | Contact email list for the account. |
| `acme-retry-wait` | Seconds between retries on failure. |
| `acme-renew-period` | Days between renewal checks. |

After `addOptions`, the user-supplied values are read on `update()`.

## Multiple domains / multiple certs

An Account manages a list of `KeyCert`s.  Each can cover one or
more domains (Subject Alternative Names).  Add them with
`acme.add(kc)`; each renews independently.

```cpp
auto a = cb::SmartPtr(new cb::ACMEv2::KeyCert("api.example.com", apiKey));
auto b = cb::SmartPtr(new cb::ACMEv2::KeyCert("www.example.com", wwwKey));
b->addDomain("example.com");        // SAN
acme.add(a);
acme.add(b);
```

`KeyCert::expiredIn(secs)` lets you check whether a cert will expire
within a window — useful for monitoring.

## CSR customization

The default `KeyCert::makeCSR()` produces a CSR with the configured
domains as SANs and the embedded `KeyPair` as the subject key.
Override it in a subclass for custom extensions:

```cpp
class MyKeyCert : public cb::ACMEv2::KeyCert {
public:
  cb::SmartPointer<cb::CSR> makeCSR() const override {
    auto csr = KeyCert::makeCSR();
    // Set additional extensions ...
    return csr;
  }
};
```

## Account key persistence

The account key must be the same across restarts; otherwise the CA
sees you as a new account and may rate-limit.

```cpp
// First run:
key.generateRSA(2048);
SystemUtilities::oopen("acme-account.pem")->write(key.privateToPEMString());
acme.setKey(key);

// Subsequent runs:
key.readPrivatePEM(SystemUtilities::read("acme-account.pem"));
acme.setKey(key);
```

The per-`KeyCert` private key can be rotated; the account key cannot
without losing your CA-recognised identity (which is mostly a
limit-tracking inconvenience, not a functional break).

## Common pitfalls

- **No HTTP on port 80.**  HTTP-01 requires the CA to reach
  `http://yourdomain/.well-known/acme-challenge/...` on plaintext
  port 80.  Don't redirect that path to HTTPS — serve it directly.
- **Wildcard certs.**  HTTP-01 cannot validate wildcard SANs; you
  need DNS-01.  cbang doesn't ship a DNS-01 plugin out of the box;
  for wildcards, use a separate ACME tool and feed the chain in
  manually.
- **Running against production while testing.**  Let's Encrypt
  rate-limits production heavily.  Always use
  `letsencrypt_staging` until your wiring is verified end-to-end.
- **Account key loss.**  Without the original account key, you
  re-register and start fresh.  Persist it.
- **Cert key reuse across renewals.**  Cleaner to generate a fresh
  cert key per renewal; `KeyCert` retains the same key by default,
  so explicitly regenerate if you want rotation.

## End-to-end example

```cpp
cb::Event::Base   base;
auto              sslCtx = cb::SmartPtr(new cb::SSLContext);
cb::HTTP::Client  client(base, sslCtx);
cb::HTTP::Server  http(base);
cb::HTTP::Server  https(base, sslCtx);
cb::ACMEv2::Account acme(client);

// Load or generate account key
cb::KeyPair acctKey;
if (SystemUtilities::exists("acct.pem"))
  acctKey.readPrivatePEM(SystemUtilities::read("acct.pem"));
else {
  acctKey.generateRSA(2048);
  SystemUtilities::oopen("acct.pem")->write(acctKey.privateToPEMString());
}
acme.setKey(acctKey);
acme.setContactEmails("ops@example.com");

// One domain
cb::KeyPair certKey;
certKey.generateRSA(2048);
auto kc = cb::SmartPtr(new cb::ACMEv2::KeyCert("example.com", certKey));
acme.add(kc);

// Wire the challenge handler
http.addMember(cb::HTTP::HTTP_GET,
  "/\\.well-known/acme-challenge/.*",
  &acme, &cb::ACMEv2::Account::challengeRequest);

// Re-install on each cert event
acme.addListener([&] (cb::ACMEv2::KeyCert &kc) {
  sslCtx->useCertificateChain(kc.getChain());
  sslCtx->usePrivateKey(kc.getKey());
});

// Bind ports
http.addListenPort(cb::SockAddr::parse("0.0.0.0:80"));
https.addSecureListenPort(cb::SockAddr::parse("0.0.0.0:443"));
http.init();
https.init();

acme.update();
base.dispatch();
```

## See also

- `cbang/acmev2/Account.h`, `cbang/acmev2/KeyCert.h` for the full
  API.
- `cbang/openssl/SSLContext.h` for installing the issued chain.
- [WebServer.md](WebServer.md) — the HTTP server that hosts the
  challenge endpoint.
- [EventSystem.md](EventSystem.md) — the loop that drives the
  state machine.
- RFC 8555 for the ACMEv2 protocol semantics.
