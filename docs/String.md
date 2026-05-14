# cbang String Utilities

This document covers `cbang/String.h` — `cb::String`'s static utility methods
for formatting, parsing, tokenizing, transforming, and inspecting
`std::string`.  Also covered: the `CBANG_SSTR` / `SSTR` stream-formatting
macro used throughout cbang.

## Overview

`cb::String` is a bag of static methods.  It is not instantiated; it groups
together the string utilities cbang needs that aren't in `<string>` or
`<algorithm>`.  The goal is to keep code that handles strings short and
explicit:

```cpp
auto port = String::parseU16(req.getArg("port"));
if (!String::startsWith(path, "/api/")) THROWX("Bad path", HTTP_NOT_FOUND);
LOG_INFO(1, "Joined: " << String::join(parts, ", "));
```

Many cbang APIs already return formatted strings (logging, exceptions, JSON
output), so `cb::String` is most useful at parse/format boundaries — reading
configuration, building log messages, processing user-supplied data.

## Headers

```cpp
#include <cbang/String.h>     // class cb::String
#include <cbang/SStream.h>    // CBANG_SSTR / SSTR macro
```

`SStream.h` is included by `Throw.h`, so any file that throws already has the
`SSTR` macro available.

When `USING_CBANG` is defined, `SSTR` is the short form of `CBANG_SSTR`.

## Formatting

### `printf`

```cpp
std::string s = String::printf("speed=%.2f%% [%llu/%llu]", pct, done, total);
```

`String::printf(fmt, ...)` and `String::vprintf(fmt, va_list)` return a
`std::string`.  They use the standard C format-string syntax and are
annotated with `[[gnu::format(printf, ...)]]` so the compiler will check
argument types.

Use `printf` when you need C-style formatting (`%x`, `%.3f`, width
specifiers).  For most other cases prefer stream formatting:

### `SSTR` — stream-to-string

```cpp
std::string s = SSTR("foo=" << foo << ", bar=" << bar);
```

`SSTR(expr)` expands to a single ostringstream expression that streams `expr`
and returns `.str()`.  It is the building block of every cbang macro that
takes a "message stream" — `THROW`, `LOG_*`, `CATCH(level, ...)`, etc. — so
the formatting rules are the same.

`SSTR` produces a single allocation; use it when you want a string but don't
need printf-style format codes.

### `bar`

```cpp
LOG_INFO(1, String::bar("Begin upload"));
// **** Begin upload ***************************************************
```

Useful for visually separating sections in long logs.  Defaults to 80 columns,
`*` as the fill character.

### `hexdump`

```cpp
LOG_DEBUG(3, "Packet:\n" << String::hexdump(buf, len));
```

Returns a multi-line hex/ASCII dump similar to `xxd`.  Three overloads accept
`const char *`, `const uint8_t *`, or `const std::string &`.

### `hexEncode` / `hexNibble`

```cpp
std::string s = String::hexEncode(digest);       // "deadbeef..."
char c = String::hexNibble(0xA, /*lower=*/false); // 'A'
```

Hex-encodes binary data.  For the inverse (binary to/from hex with parsing),
see `cbang/util/Base64.h` and `cbang/openssl/Digest.h` which have dedicated
helpers.

## Parsing

### Typed numeric/boolean parsers

For each of `S8`, `U8`, `S16`, `U16`, `S32`, `U32`, `S64`, `U64`, `Double`,
`Float`, `Bool` there are three forms:

```cpp
static TYPE parseNAME(const std::string &s, TYPE &value, bool full);
static TYPE parseNAME(const std::string &s, bool full = false);
static TYPE isNAME(const std::string &s, bool full);
```

  - `parseNAME(s)` — returns the parsed value; throws `cb::ParseError` on
    failure
  - `parseNAME(s, &value, full)` — out-parameter form; `full=true` requires
    the entire string to be consumed (no trailing junk)
  - `isNAME(s, full)` — type-test predicate

Examples:

```cpp
uint16_t port = String::parseU16(arg);                // throws on bad input
double t      = String::parseDouble("3.14e-2");
bool   flag   = String::parseBool("yes");             // accepts y/yes/1/true
if (String::isU32(s, /*full=*/true)) ...
```

`parseBool` accepts the obvious set: `"true"`, `"false"`, `"yes"`, `"no"`,
`"on"`, `"off"`, `"1"`, `"0"` (case-insensitive).

### Generic `parse<T>`

```cpp
auto n = String::parse<uint32_t>(s, /*full=*/true);
bool ok = String::parse(s, n, /*full=*/true);  // non-throwing, returns success
```

Useful in templated code.

### `isInteger` / `isNumber`

Non-typed predicates: does `s` parse as some integer / some number.  Cheaper
than calling all the typed forms when you only need a yes/no answer.

## Tokenizing

```cpp
std::vector<std::string> parts;
String::tokenize("a, b,, c", parts, ", ");           // {"a", "b", "c"}
String::tokenize("a, b,, c", parts, ", ", /*allowEmpty=*/true);
// {"a", "", "b", "", "", "c"}
String::tokenize(input, parts, ",", false, /*maxTokens=*/3);
```

Signature:

```cpp
static unsigned tokenize(const std::string &s,
                         std::vector<std::string> &tokens,
                         const std::string &delims = DEFAULT_DELIMS,
                         bool allowEmpty = false,
                         unsigned maxTokens = ~0);
```

`DEFAULT_DELIMS` is whitespace.  Returns the number of tokens added.
`maxTokens` is an upper bound; the rest of the string is left as the final
token (useful when parsing "key: value" where the value may contain the
delimiter).

`tokenizeLine(stream, tokens, delims, lineDelims, maxLength)` reads one line
from an `istream` and tokenizes it.  Use it for CSV/TSV-style ingestion.

## Transformations

```cpp
String::trim(s);                  // removes leading + trailing whitespace
String::trimLeft(s, " \t");       // custom delimiter set
String::trimRight(s);

String::toUpper("Foo");           // "FOO"
String::toLower("Foo");           // "foo"
String::capitalize("foo bar");    // "Foo bar"  (first char only)

String::startsWith(path, "/api/");
String::endsWith(name, ".tar.gz");

String::replace("a.b.c", '.', '_');           // char form: "a_b_c"
String::replace("Hello World", "World", "!"); // string form: regex search
```

Note: the string-form `replace` uses regular-expression search — pass an
escaped pattern (`String::escapeRE`) if you mean literal replacement of a
multi-character string.

`fill(s, currentColumn, indent, maxColumn)` reflows `s` to fit within
`maxColumn` characters, optionally indenting continuation lines.  Used by the
options system to format help text.

`ellipsis(s, width)` truncates long strings and appends `…` if they exceed
`width`.

`transcode(s, from, to)` converts between character encodings via iconv (only
useful when working with non-UTF-8 sources).

## Escaping

```cpp
String::escapeC("line1\nline2\t\"quoted\"");
// returns: line1\nline2\t\"quoted\"
String::unescapeC(s);

String::escapeRE("a.b*c");  // "a\\.b\\*c"
```

`escapeC` produces C-string-literal escapes (`\n`, `\t`, `\"`, `\xff` for
non-printable).  Two overloads write to a `std::string` result by reference,
which is useful inside tight loops.

`escapeRE` escapes characters that have special meaning in regular
expressions.

## Joining containers

Three overloads of `String::join`:

```cpp
String::join(begin, end, ", ");                       // iterator range
String::join(vec, ", ");                              // any container
String::join(stream, vec, ", ");                      // stream variant
```

The container form requires `vec.begin()` and `vec.end()`; the stream form
avoids an intermediate string when you're going to write the result to an
`ostream` anyway (logging, files, sockets).

Elements are streamed via `operator<<`, so they can be any type that
ostreams know how to print.

## Patterns

### Parsing user-supplied numbers

```cpp
auto raw = req.getArg("limit");
uint32_t limit;
if (!String::parse(raw, limit, /*full=*/true) || limit > MAX_LIMIT)
  THROWX("Invalid limit: " << raw, HTTP_BAD_REQUEST);
```

The `full=true` flag prevents `"42xyz"` from silently parsing as `42`.

### Building log lines

Prefer streaming via `LOG_*` macros over `printf` — they avoid building the
string at all when the level is filtered out:

```cpp
LOG_INFO(2, "Worker " << id << " done in " << ms << "ms");
```

Use `String::printf` only when you need exact formatting (fixed-width
columns, hex padding).

### Pre-checking before parse

If you frequently parse the same value during validation:

```cpp
if (!String::isU16(port, /*full=*/true))
  THROWX("Bad port: " << port, HTTP_BAD_REQUEST);
auto p = String::parseU16(port);
```

Strictly speaking, parsing once and catching `ParseError` is fewer operations,
but the predicate form is clearer when the failure path produces a
domain-specific error.

### Stripping a known prefix/suffix

```cpp
if (String::startsWith(name, "_temp_"))
  name = name.substr(6);
```

No dedicated `stripPrefix` helper; this is the idiom.

## Common pitfalls

  - **`replace(s, "from", "to")` is regex-based.**  Special characters in
    `"from"` are interpreted; escape with `escapeRE` if you want literal
    semantics, or use the char-overload for single characters.
  - **`parseBool` accepts more than `"true"`/`"false"`.**  See list above.
    For strict JSON-style booleans, parse via `cb::JSON::Reader` instead.
  - **`tokenize` defaults to skipping empty tokens.**  Pass `allowEmpty=true`
    when consecutive delimiters are meaningful (CSV columns).
  - **`SSTR` is a macro.**  Don't pass an expression with side effects you
    care about — it expands to a one-shot ostringstream temporary.  Also,
    `operator<<` precedence means you sometimes need parentheses:
    `SSTR("flag=" << (a && b))`.
  - **`printf` returns a `std::string`, not `void`.**  Don't confuse it with
    `std::printf` in code that mixes both.
  - **`escapeC` is for C-string literals**, not URL/XML/HTML escaping.  Use
    the appropriate codec from `cbang/util/` (`Base64`, `URI`) or
    `cbang/xml/` for those.

## See also

  - [Exceptions.md](Exceptions.md) — `THROW`/`THROWX` use `SSTR` internally
  - [Logging.md](Logging.md) — `LOG_*` macros use the same stream syntax
  - [Config.md](Config.md) — option parsing builds on `parseU32`, `parseBool`
  - [JSON.md](JSON.md) — for JSON-format-compliant parsing of numbers/bools
