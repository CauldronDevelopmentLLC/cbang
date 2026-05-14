---
layout: home

hero:
  name: "C!"
  text: "Cross-platform C++ utility library"
  tagline: "Also known as cbang. Clean, modular C++ infrastructure refined across more than two decades of production use."
  image:
    src: /cbang.png
    alt: cbang
  actions:
    - theme: brand
      text: Read the Docs
      link: /Application
    - theme: alt
      text: View on GitHub
      link: https://github.com/CauldronDevelopmentLLC/cbang

features:
  - title: Event-Driven HTTP / HTTPS / WebSocket
    details: >-
      Async server and client built on libevent. TLS via OpenSSL, automatic
      Let's Encrypt certificates, request routing, WebSocket framing, content
      compression, and streaming responses.
    link: /WebServer
    linkText: WebServer docs
  - title: JSON with Observables
    details: >-
      Reader, Writer, and a typed JSON value tree. Observable subtrees notify
      subscribers on change — the foundation of cbang's reactive state model.
    link: /JSON
    linkText: JSON docs
  - title: Application Framework
    details: >-
      cb::Application + doApplication&lt;T&gt; wires options, logging,
      configuration, the event loop, signal handling, and a top-level catch
      into one main().
    link: /Application
    linkText: Application docs
  - title: Smart Pointers
    details: >-
      Intrusive reference counting via RefCounted, weak pointers, downcasting,
      phony pointers, and WeakCall — the SmartPointer toolkit the rest of
      cbang is built on.
    link: /SmartPointer
    linkText: SmartPointer docs
  - title: Rich Exception System
    details: >-
      cb::Exception with cause chains, file/line capture, numeric codes (mapped
      to HTTP status), and optional stack traces. THROW / CATCH / ASSERT
      macros, plus predefined subclasses.
    link: /Exceptions
    linkText: Exception docs
  - title: Configuration & Options
    details: >-
      Strongly typed options with command-line, environment, and config-file
      sources. Auto-generated --help, deprecation, action callbacks, and JSON
      config dumps.
    link: /Config
    linkText: Config docs
  - title: Async DNS
    details: >-
      Non-blocking DNS resolver integrated with the event loop. Supports A,
      AAAA, SRV, MX, TXT, and CNAME queries with caching and retries.
    link: /AsyncDNS
    linkText: AsyncDNS docs
  - title: Let's Encrypt (ACMEv2)
    details: >-
      Automatic TLS certificate acquisition and renewal. HTTP-01 challenge
      handler runs alongside your existing web server with zero downtime
      rotation.
    link: /ACMEv2
    linkText: ACMEv2 docs
  - title: SQL — SQLite & MariaDB
    details: >-
      Idiomatic C++ wrappers around both. Prepared statements, parameter
      binding, RAII transactions, and an async MariaDB client that integrates
      with the event loop.
    link: /SQLite
    linkText: Database docs
  - title: OpenSSL C++ Wrapper
    details: >-
      Modern C++ over libssl/libcrypto. Certificates, keys, digests, ciphers,
      JWTs, and ACMEv2 — without the C boilerplate.
    link: /OpenSSL
    linkText: OpenSSL docs
  - title: API Framework
    details: >-
      Declarative HTTP/JSON API definition with validation, role-based access,
      WebSocket subscriptions, and automatic OpenAPI-style introspection.
    link: /API
    linkText: API docs
  - title: Networking Primitives
    details: >-
      SockAddr (IPv4/IPv6/Unix), URI parser, CIDR ranges, allow/deny filters,
      Base64 codecs, and the rest of the low-level net toolkit.
    link: /Net
    linkText: Net docs
---

## Overview

The C! library (a.k.a. **cbang** or *C Bang*) is a collection of C++
utilities developed across more than twenty years of production application
work. It compiles and runs on Linux, macOS, and Windows with a modern C++
compiler.

cbang's design priorities are clarity, modularity, and reusability. It
prefers exception-based error handling, makes light use of templates, and
keeps preprocessor macros to the small set that genuinely improve the call
site (`THROW`, `LOG_*`, `CATCH_ERROR`, `CBANG_ENUM`, …).

The library is the foundation of the
[Folding@home client](https://github.com/FoldingAtHome/fah-client-bastet),
the [CAMotics CNC simulator](https://camotics.org/), and a number of other
applications that need cross-platform C++ infrastructure without pulling in
a heavy framework.

## Why cbang?

- **Production-tested.** Running on hundreds of thousands of Folding@home
  installations for years.
- **Event-driven by default.** A real async HTTP/HTTPS/WebSocket stack on
  libevent — not a toy.
- **Batteries included.** JSON, SQL, TLS, DNS, ACME, logging, options,
  exceptions, smart pointers — all consistent, all interoperable.
- **Readable.** Clean code, conservative templates, minimal macros.
- **Cross-platform.** Linux, macOS, Windows.

## Getting Started

cbang is built with [SCons](https://scons.org/). Build instructions live in
the [project README](https://github.com/CauldronDevelopmentLLC/cbang#readme).

Once built, link against `libcbang` and start with the
[Application framework](/Application) — it sets up logging, options, and
the event loop in a few lines.

## License

cbang is released under the
[GNU Lesser General Public License v2.1](http://www.gnu.org/licenses/lgpl-2.1.html)
or, at your option, any later version. Documentation is licensed under
[CC BY 3.0](http://creativecommons.org/licenses/by/3.0/).

Commercial / BSD-style licensing may be granted on a case-by-case basis;
contact [joseph@cauldrondevelopment.com](mailto:joseph@cauldrondevelopment.com).
