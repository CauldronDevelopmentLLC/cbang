# C! — C++ Coding Style

This is a style guide for use in the C! codebase.  Follow this document when adding or updating code in C! or its associated projects.

C! maintains a strict coding style.  This may seem tedious to some, but strict discipline enhances maintainability and ultimately saves time.  Some of the style choices are not conventional, but each choice is made for a good reason.  C!'s style enhances readability by fitting more code vertically and improves the developer's ability to compare code with the 80-column limit, which allows up to three files to be viewed side-by-side on a modern screen.

## Guiding principles:
- Consistency
- Add just enough space to enhance readability
- Avoid unnecessary syntax
- Don't Repeat Yourself (DRY)
- Don't add code you might need later

## Files
- C++ headers use the ``.h`` extension.
- C++ implementation files use the ``.cpp`` extension.
- X-Macro definitions use the ``.def`` extension.
- C++ header and implementation files must match the C++ class name.
- There should be one header and one implementation file for each non-trivial class.

## Header
- Use `#pragma once` instead of `#ifdef NAME_H` in headers.

## Includes
- Order includes:
  1. Local header includes with double quotes (e.g. `"SmartPointer.h"`).
  2. C! project headers `<cbang/...>` (when applicable).
  3. C++ standard headers (`<vector>`, `<string>`, ...).
  4. C headers, prefer C++ versions.
- Keep include lists grouped and separated by a blank line.

Example:
```c++
#include "ColumnDef.h"
#include "Statement.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/StringMap.h>

#include <vector>

#include <unistd.h>
```

## Naming conventions
- Enums / classes / structs: PascalCase (e.g., `TableDef`, `PCIInfo`).
- Member variables, functions and methods: lowerCamelCase (e.g., `dbName`, `maxKeyLength`).
- Alias names: end with `_t` (e.g., `columns_t`).
- Macros and preprocessor symbols: ALL_CAPS with underscores.

## Indentation, line length & whitespace
- Indent with 2 spaces.
- Do not use tabs.
- Enforce a strict 80 column limit. Break long lines at logical boundaries.
- Do not leave extra whitespace at the end of lines.

## Spacing: Operators, parentheses, array indices, commas, semicolons and line breaks
- Binary operators
  - Place one space before and after binary operators: `a + b`, `x == y`, `i <= n`.
- Unary operators
  - No space between a unary operator and its operand: `-x`, `!flag`, `++i`, `--i`.
- Pointer/reference declarations
  - Space between type and `*`/`&`, and no space between symbol and variable name:
    - `BIGNUM *bn;`
    - `const std::string &s;`
  - In expressions, write `*ptr` and `&obj` (no space).
- Function calls and declarations
  - No space between a function name and the opening parenthesis: `foo(x, y)`.
  - Space between control keyword and `(`: `if (cond)`, `for (int i = 0; i < n; i++)`, `while (expr)`.
  - Do not add spaces immediately inside parentheses: `f(a, b)`, not `f( a, b )`.
- Parentheses and casts
  - Use `static_cast<T>(expr)` etc., with no extra spaces inside `< >` or `( )`.
- Array subscripts
  - No space before `[` or between brackets and index: `arr[i]`, `buf[0]`.
- Commas
  - No space before a comma, a single space after: `f(a, b, c)`.
  - In long argument lists, break after a comma.
- Semicolons
  - No space before `;`. In `for` headers, use a single space after each `;` component: `for (int i = 0; i < n; i++)`.
- Member access
  - No spaces around `.` or `->`: `obj.method()`, `ptr->field`.
- Ternary operator
  - Use single spaces around `?` and `:`: `cond ? a : b`.
- Colons (labels, constructor init)
  - In constructor initializer lists keep a single space before and after `:`:
    - `Info(Inaccessible) : maxKeyLength(0) {}`
- Templates and angle brackets
  - No spaces immediately inside angle brackets: `std::vector<int>`.
  - Space after commas in template parameter lists: `std::map<Key, Value>`.
- Range-based for
  - Single space around `:`: `for (auto &v : container)`.
- Line breaks and continuations
  - Prefer breaking at commas or open parenthesis when a line is long.
  - Indent continuation lines by 2 spaces relative to the previous block.

## Function spacing
- There should be blank lines between functions.
- Single-line functions and methods have no space between them.

## Braces and control statements
- Opening braces on the same line.
- Place `else` and `else if` on same line as the preceding brace.
- Add a blank line before a `} else` but not before a bare `else`.
- Do not add braces for a single child statement unless it prevents a dangling else.
```c++
if (a) {
  foo();
  bar();

} else if (b) {
  baz();
  while (true) bla();

} else bas();
```
```c++
if (a) {
  if (b) foo();
  else bar();
}
```

## Single-line vs multi-line function bodies
- Small single-line single-statement functions should be written on one line. Place the statement immediately after `{` with no leading space:
```c++
iterator begin() const {return devices.begin();}
```
- For multi-line functions, put each statement on its own line and indent:
```c++
void URI::parsePair(const char *&s) {
  string name  = parseName(s);
  string value = consume(s, '=') ? parseValue(s) : "";
  set(name, value);
}
```

## Class member initializers and constructors
- Use initializer lists; place a single space before `:` and a single space after `:` before initializers:
```c++
Info(Inaccessible) : maxKeyLength(0) {}
```
- Initializer order must match declaration order.

## Access control & order in classes
- Default access is private. Common layout:
  1. Data members (private by default)
  2. typedefs / using aliases
  3. public: interface
  4. protected: helpers (when needed)

## Pointers, references
- Place pointer `*` and reference `&` next to the variable name, leaving a space between the type and the symbol:
```c++
BIGNUM *bn;
const std::string &s;
```

## NULL pointers
- Use `0` for null pointer literals; do not use `NULL` or `nullptr`.

## Avoid `x != 0` and `x == 0`
- Use `if (x)` or `if (!x)` instead of `if (x != 0)` or `if (x == 0)`.

## Avoid `>` and `>=`
- Prefer `<` and `<=` operators.

## Do not add unnecessary parentheses
- Do not wrap return expressions; e.g. `return x + y;` not `return (x + y);`
- Do not wrap operators when precedence rules are sufficient; e.g. `if (x * y == 5)` not `if ((x * y) == 5)`.

## Indices, sizes and integer types
- Use `unsigned` for counts and non-negative sizes in the API (the repo commonly uses `unsigned`).
- Use fixed-width integers (`uint16_t`, `uint8_t`, `uint64_t`, etc.) when interfacing with hardware, binary formats, or external protocols.

## Typedefs, aliases and iterator naming
- Use `using` rather than `typedef`:
```c++
using iterator = ListImpl::const_iterator;
```

## Virtual functions & overrides
- Use `override` for overridden virtual functions:
```c++
void callback(state_t state) override;
```

## Error handling
- Use exceptions for error reporting in library code.
- Keep error messages clear, and avoid swallowing exceptions silently.

## Macros & preprocessor
- Minimize macros.
- Macro names upper-case, and clearly documented.
- When generating code with macros, provide a corresponding cleanup section (undef).

## Namespaces
- Do not add ``using`` namespace directives in headers.
- All code must be inside a namespace.
- Use anonymous namespaces instead of `static` for internal linkage:
```c++
namespace {
  void foo() {...}
  void bar() {...}
}
```

## Const
- Mark functions `const` where applicable.

## Auto
- Use `auto` keyword to replace complicated type declarations where possible.

## Logging
- Use C!'s `LOG_*` macros.

## Threads, locking and events
- Avoid threads.
- Use the event system.
- Threading is only acceptable for backgrounding CPU intensive tasks.
- Avoid locking, use events and message passing.

## Compiler warnings
- The code should not produce any compiler warnings with the default settings.

## Comments & documentation
- Prefer concise inline comments that explain "why", not "what".
- Don't waste the reader's time.
- Every source/header file begins with a license/copyright block.

## Testing
- Add unit tests to prevent future regressions.
- Any changes must pass or fix all unit tests.
- Test one thing at a time, within reason.
- Each test must complete quickly.