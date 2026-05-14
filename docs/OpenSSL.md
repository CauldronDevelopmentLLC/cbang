# cbang OpenSSL Wrapper

cbang wraps OpenSSL into a higher-level C++ API: key pairs,
certificates, certificate chains, CSRs, digests, ciphers, BigNums,
SSL contexts, and a few stream filters for I/O.  All operations
throw `cb::Exception` on failure with the OpenSSL error stack
attached.

The wrapper is used by [WebServer.md](WebServer.md) (TLS),
[ACMEv2.md](ACMEv2.md) (cert acquisition), and any app that
verifies signatures, hashes data, or talks SSL.

## Concepts

- **`cb::KeyPair`** — an asymmetric key (RSA, DSA, EC, DH).  Holds
  the private key by default; `publicTo*` methods extract the
  public-only form.  Used for signing, verification, and as the
  identity in certificates.
- **`cb::Certificate`** — an X.509 certificate.  Reads/writes PEM,
  exposes subject/issuer/extensions, signs/verifies against keys.
- **`cb::CertificateChain`** — an ordered chain (leaf → root).
- **`cb::CertificateStore` / `CertificateStoreContext`** — a trust
  store and per-verification context for chain validation.
- **`cb::CSR`** — a Certificate Signing Request.  Subject + SANs +
  public key, signed by the requester's private key.
- **`cb::Digest`** — message digest / HMAC.  Streaming update +
  finalize, or one-shot `Digest::hash(data, "sha256")`.
- **`cb::Cipher`** — symmetric encryption/decryption (AES, etc.).
- **`cb::BigNum`** — arbitrary-precision integers (RSA modulus,
  signatures, etc.).
- **`cb::SSLContext`** — `SSL_CTX *` wrapper.  Configure once, pass
  to `HTTP::Client` / `HTTP::Server` for TLS.
- **`cb::SSL`** — global init (`SSL::init()`).
  `cb::doApplication<T>` calls this for you.

## Key pairs

```cpp
#include <cbang/openssl/KeyPair.h>

cb::KeyPair key;
key.generateRSA(4096);                   // strong, slow
// key.generateRSA(2048);                // faster, common
// key.generateEC("prime256v1");         // ECDSA
// key.generateDSA(2048);
// key.generateDH(2048);

std::string pubPem  = key.publicToPEMString();
std::string privPem = key.privateToPEMString();

std::string spki    = key.publicToSPKI();   // for fingerprinting
```

### Read / write

```cpp
key.readPrivatePEM(SystemUtilities::read("server.key"));
key.readPublicPEM(SystemUtilities::read("server.pub"));

SystemUtilities::oopen("server.key")->write(key.privateToPEMString());
```

PEM is the typical format on disk.  DER (raw bytes) is also
available via `publicToDER()` / `privateToDER()`.

### Pacifier during keygen

RSA-4096 generation takes seconds.  Pass a `KeyGenPacifier` to print
progress dots while it runs:

```cpp
#include <cbang/openssl/KeyGenPacifier.h>
key.generateRSA(4096, 65537, new cb::KeyGenPacifier("Generating RSA key"));
```

fah-client uses this on first run (`App.cpp:449`).

### Signing and verification

```cpp
std::string sig    = key.sign(message);                  // SHA-256 by default
std::string sig64  = key.signBase64SHA256(message);       // convenience

bool ok = key.verify(sig, hashedMessage);
```

For separate `(data, signature)` verification — common in
verifying server responses — combine with `Digest::hash` and
`KeyPair::verify`:

```cpp
auto hash = Digest::hash(payload, "sha256");
bool ok   = certPubKey.verify(signature, hash);
```

### Key identity

```cpp
auto N  = key.getRSA_N();             // BigNum — the modulus
auto id = Digest::urlBase64(N.toBinString(), "sha256");
// Stable, public, hash-of-modulus identifier — used by fah-client
// as the F@H ID (App.cpp:456).
```

## Digests / hashes

```cpp
#include <cbang/openssl/Digest.h>

// One-shot
std::string h = cb::Digest::hash("hello", "sha256");        // raw bytes
std::string s = cb::Digest::hashHex("hello", "sha256");     // hex
std::string b = cb::Digest::base64(data, "sha256");
std::string u = cb::Digest::urlBase64(data, "sha256");

// Streaming
cb::Digest md("sha256");
md.update(part1);
md.update(part2);
md.finalize();
auto digest = md.toString();

// HMAC
std::string mac = cb::Digest::hmac(data, key, "sha256");
```

Algorithms accept any OpenSSL name: `"sha1"`, `"sha256"`,
`"sha512"`, `"sha3-256"`, `"md5"` (don't), etc.

## Certificates

```cpp
#include <cbang/openssl/Certificate.h>

cb::Certificate cert(pemString);          // parse from PEM
auto subj = cert.getSubject().toString();
auto pub  = cert.getPublicKey();          // KeyPair (public only)

// Extensions
auto ext  = cert.getExtension("subjectAltName", "");
auto keyUsage = cert.getExtension("fahKeyUsage", "");

// Sign / verify
cert.sign(caKey, "sha256");
cert.verify(caPublicKey);                 // throws if invalid
```

`Certificate::addExtensionAlias("fahKeyUsage", "nsComment")` lets
you reference a custom OID by a friendly name — useful when you've
defined application-specific extensions.

### Building a cert from a CSR

```cpp
cb::CSR csr;
csr.addNameEntry("CN", "example.com");
csr.addExtension("subjectAltName", "DNS:example.com,DNS:www.example.com");
csr.sign(certKey);

// Hand to your CA-side code:
cb::Certificate cert;
cert.setSubject(csr.getSubject());
cert.setPublicKey(csr.getPublicKey());
cert.setIssuer(caCert.getSubject());
// ... validity, serial, extensions ...
cert.sign(caKey);
```

## Certificate chains

```cpp
#include <cbang/openssl/CertificateChain.h>

cb::CertificateChain chain;
chain.add(intermediateCert);
chain.add(rootCert);

// Write the whole chain in PEM (concatenated):
chain.writeFile("chain.pem");
```

### Verifying

```cpp
cb::CertificateStore store;
store.add(caCert);

cb::CertificateStoreContext(store, leafCert, chain).verify();
// throws on failure
```

This is the pattern used in `App::validate` (fah-client
`App.cpp:309`): build a chain, build a store with the F@H CA, run
the context's `verify()`.

## CSRs

```cpp
#include <cbang/openssl/CSR.h>

cb::CSR csr;
csr.addNameEntry("CN", "client.example.com");
csr.addExtension("subjectAltName", "DNS:client.example.com");
csr.sign(myKey);                   // private key signs

std::string pem = csr.toPEM();
```

ACMEv2's `KeyCert::makeCSR()` returns one of these for each cert
order.

## Symmetric encryption

```cpp
#include <cbang/openssl/Cipher.h>

cb::Cipher c("aes-256-cbc", /*encrypt*/ true);
c.init(0, true, key.data(), iv.data());

std::vector<uint8_t> out(in.size() + 16);
unsigned n = c.update(out.data(), out.size(), in.data(), in.size());
n += c.finalize(out.data() + n, out.size() - n);
out.resize(n);
```

Algorithms: any OpenSSL cipher name (`"aes-256-cbc"`,
`"aes-256-gcm"`, `"chacha20-poly1305"`).

`cb::CipherStream` provides a streaming interface as a
`std::iostream` filter for use with cbang's I/O stack.

## BigNums

```cpp
#include <cbang/openssl/BigNum.h>

cb::BigNum n;
n.fromHex("1234abcd");
std::string h   = n.toHexString();
std::string bin = n.toBinString();
auto bytes      = n.size();

// Arithmetic via overloaded operators
auto sum = a + b;
```

Usually you only interact with BigNums to inspect RSA moduli or
signature values; they are mostly an internal detail.

## SSLContext (TLS configuration)

```cpp
#include <cbang/openssl/SSLContext.h>

auto ctx = cb::SmartPtr(new cb::SSLContext);
ctx->loadSystemRootCerts();
ctx->setCipherList("HIGH:!aNULL:!eNULL:!EXPORT:!DES:!MD5:!PSK:!RC4");
ctx->useCertificateChainFile("server.crt");
ctx->usePrivateKey(myKey);

// Pass to HTTP layer:
cb::HTTP::Client client(base, ctx);
cb::HTTP::Server server(base, ctx);
```

### Client / server verification

```cpp
ctx->setVerifyPeer();          // require peer cert; default-strict
ctx->setVerifyNone();          // skip verification (tests only!)
ctx->setVerifyDepth(4);
```

`setVerifyNone` disables cert validation — useful for unit tests
that don't have a real CA chain.  Don't ship a production binary
that calls it.

### Loading roots

`loadSystemRootCerts()` picks up the system trust store
(`/etc/ssl/certs` on most Linux, the system keychain on macOS, the
Windows cert store).  Call this once on the context before any TLS
operations.

You can also hard-code a CA by reading a PEM and using
`useCertificateChain` / `addCert`.

## SSL globals

```cpp
#include <cbang/openssl/SSL.h>
cb::SSL::init();
```

`cb::doApplication<T>` calls this for you.  If you skip
`doApplication` (e.g. embedded use) you must call it yourself
before any TLS or crypto.

`SSL::createObject(oid, sn, ln)` registers a custom OID — useful
for application-specific certificate extensions like fah-client's
`fahKeyUsage` (`App.cpp:203`).

## Stream filters

For piping data through OpenSSL transformations without manual
update/finalize loops:

- **`cb::BIStream`** / **`cb::BOStream`** — `std::istream`/`std::ostream`
  backed by an OpenSSL BIO.
- **`cb::DigestStreamFilter`** — hash as data flows through.
- **`cb::CipherStream`** — encrypt/decrypt streaming I/O.

Useful when computing a digest over data you'd otherwise have to
buffer entirely.

## Patterns

### Sign a payload, send with signature

```cpp
std::string payload   = builder.toString();
std::string signature = key.signSHA256(payload);
std::string sig64     = cb::Base64().encode(signature);

req->send([&] (cb::JSON::Sink &sink) {
  sink.beginDict();
  sink.insert("data",      payload);
  sink.insert("signature", sig64);
  sink.insert("pub-key",   key.publicToPEMString());
  sink.endDict();
});
```

The fah-client AS-request pattern.

### Verify a signed AS/WS response

```cpp
void App::check(const std::string &certPem, const std::string &interPem,
                const std::string &sig, const std::string &hash,
                const std::string &usage) {
  cb::Certificate cert(certPem);

  // Chain validation
  if (interPem.empty()) validate(cert);
  else validate(cert, cb::Certificate(interPem));

  // Application-specific extension check
  if (!hasFAHKeyUsage(cert, usage))
    THROW("Certificate not valid for F@H key usage " << usage);

  // Signature check
  cert.getPublicKey().verify(sig, hash);
}
```

### Compute a stable identifier

```cpp
std::string id = cb::Digest::urlBase64(
                   key.getRSA_N().toBinString(), "sha256");
```

Hash of the public key modulus → URL-safe base64.  Stable across
runs as long as the key doesn't change.  fah-client uses this as
its F@H ID (`App.cpp:456`).

### Build an HTTPS server

```cpp
auto ctx = cb::SmartPtr(new cb::SSLContext);
ctx->useCertificateChainFile("server.crt");
ctx->usePrivateKey(key);

cb::HTTP::Server server(base, ctx);
server.addSecureListenPort(cb::SockAddr::parse("0.0.0.0:443"));
server.init(opts);
```

### Generate and persist an account key

```cpp
cb::KeyPair k;
if (SystemUtilities::exists("account.key"))
  k.readPrivatePEM(SystemUtilities::read("account.key"));
else {
  k.generateRSA(4096, 65537, new cb::KeyGenPacifier("RSA"));
  SystemUtilities::oopen("account.key")->write(k.privateToPEMString());
}
```

## Common pitfalls

- **Forgetting `SSL::init()`.**  Outside of `doApplication`, calls
  fail with cryptic errors.  Always init first.
- **Reusing a `Cipher` after `finalize()`** without `init`.  The
  state is consumed; create or re-init.
- **Calling `verify()` and ignoring the return.**  Some overloads
  throw, others return `bool`.  Read the signature carefully.
- **Confusing public-only and full key.**  `publicToPEMString`
  emits the public half; `privateToPEMString` emits both (private
  contains public).  Don't ship the private key by accident.
- **Loading system roots once per request.**  Heavy; load once on
  the context, share it across all clients/servers.
- **Slow RSA-4096 generation.**  Use a pacifier and cache the
  generated key.
- **Custom extensions without `createObject` / `addExtensionAlias`.**
  `getExtension("fahKeyUsage")` returns empty if the OID isn't
  registered before parsing.
- **`setVerifyNone()` in production.**  Defeats TLS.  Use it only
  in offline tests.

## See also

- `cbang/openssl/KeyPair.h`, `Certificate.h`,
  `CertificateChain.h`, `CertificateStore.h`,
  `CertificateStoreContext.h`, `CSR.h`, `Digest.h`,
  `Cipher.h`, `BigNum.h`, `SSLContext.h`, `SSL.h`.
- [WebServer.md](WebServer.md) — TLS for HTTP/WS.
- [ACMEv2.md](ACMEv2.md) — automated cert acquisition that uses
  this API.
- [Application.md](Application.md) — `cb::doApplication` calls
  `SSL::init()` for you.
- fah-client-bastet `App.cpp` (RSA key gen + persistence,
  `App::validate`/`check`/`hasFAHKeyUsage`, custom extension
  registration), `Unit.cpp` (signed AS requests).
