# testHarness

Automated tests let you change code with confidence: each test pins
down a specific behavior so that any later change that breaks it is
caught immediately.  testHarness is built to make tests cheap to write
and easy to maintain in large numbers.

It implements *golden-file* (a.k.a. *snapshot*) testing: each test
case runs a program with deterministic input and diffs its stdout,
stderr, and exit code against expected output committed alongside the
code.  This style fits programs with structured, reproducible output —
parsers, formatters, code generators, command-line tools — where
writing a test is mostly running the program once and inspecting the
captured output before committing it.

Tests are language-agnostic — the program under test can be a C++ binary,
a shell script, a Python script, anything that takes input and produces
output.

## Quick start

From a directory containing a `*Tests/` subtree:

```sh
./testHarness                 # run everything
./testHarness run JSONTests   # run one suite
./testHarness run JSONTests/DictTest    # run one case
./testHarness diff JSONTests/DictTest   # show diff after a failure
./testHarness init JSONTests/DictTest   # save current output as expected
./testHarness -I              # interactive: prompt on each failure
```

Tests run sequentially; there is no parallelism.  Total wall time
scales with the number of cases.

## Directory layout

```
tests/                           # cwd when invoking testHarness
├── testHarness                  # the runner itself
├── SConstruct                   # (optional) build for the test programs
└── <name>Tests/                 # one test suite — directory name ends in "Tests"
    ├── SConscript               # (optional) builds programs for this suite
    ├── test.json                # suite defaults (command, etc.)
    ├── <prog>.cpp               # (optional) source for the program under test
    ├── <prog>                   # the built program
    └── <case>Test/              # one test case — directory name ends in "Test"
        ├── test.json            # case-specific config (args, command override)
        ├── data/                # inputs supplied to the test
        │   ├── stdin            #   piped to test stdin
        │   ├── args             #   used as argv if no "args" key set
        │   └── <other>          #   copied into the run dir
        ├── expect/              # expected outputs
        │   ├── stdout
        │   ├── stderr
        │   └── return
        ├── run/                 # created at run time; cwd of the test
        └── disable              # touch this file to skip the test
```

Discovery is by naming convention: `*Tests/` directories are suites,
`*Test/` directories inside them are cases.

## How a test runs

For each case the harness:

1. Reads the suite's `test.json`, then merges the case's `test.json` on top.
2. Creates the run directory `<case>Test/run/`.
3. Copies files matching the `copy` pattern (default `data/*`) into the run dir.
4. Builds the command line:
   - the `command` value (suite or case `test.json`)
   - plus the `args` value, OR the contents of `data/args` if no `args` set.
   - Variables (`%(suite-dir)s`, etc.) are interpolated.
5. Runs the command with cwd set to the run dir.  Pipes `data/stdin`
   (or the `stdin` config string) to its standard input.
6. Captures the command's stdout, stderr, and exit code as files in
   `run/stdout`, `run/stderr`, `run/return`.
7. Applies any configured filters to `run/stdout` / `run/stderr`.
8. Diffs the captured files against `expect/stdout`, `expect/stderr`,
   `expect/return`.
9. Reports PASSED or FAILED.  If FAILED, the reason is the name of the
   first mismatched file.

## Walkthrough: writing a new test

Suppose you want to test `/bin/echo`.

**1. Create the suite directory** with a `test.json` setting the command:

```
echoTests/
└── test.json
```

```json
{
  "command": "echo"
}
```

**2. Create a case directory** with arguments and an empty expect dir:

```
echoTests/HelloTest/
├── test.json
└── expect/
```

`echoTests/HelloTest/test.json`:

```json
{
  "args": ["hello"]
}
```

**3. Initialize the expected output:**

```sh
./testHarness init echoTests/HelloTest
```

This runs the test once and captures stdout/stderr/return as the
expected output.  Inspect `expect/` to confirm it looks right, then
commit.

**4. Run the test going forward:**

```sh
./testHarness run echoTests/HelloTest
```

Any deviation from the saved output now fails the test.

## test.json reference

All keys are optional unless noted.  Case-level values override
suite-level values.

| Key | Type | Description |
|---|---|---|
| `command` | string or list | The program to run.  Required (suite or case). |
| `args` | string or list | Appended to the command. |
| `stdin` | string | Literal stdin content.  Overrides `stdin_file`. |
| `stdin_file` | path | File piped to stdin.  Default: `%(data-dir)s/stdin`. |
| `args_file` | path | File whose contents are used as args.  Default: `%(data-dir)s/args`. |
| `copy` | glob pattern | Files copied into the run dir.  Default: `%(data-dir)s/*`. |
| `env` | object | Extra environment variables.  Merged with parent suite env. |
| `timeout` | number | Per-test timeout in seconds.  Default: 300. |
| `shell` | boolean | Run via the shell (`shell=True`).  Default: false. |
| `checks` | list | Override the default stdout/stderr/return diff.  See [Checks](#checks). |
| `tests` | list | Override which `*Test/` directories are picked up. |
| `suites` | list | Override which `*Tests/` subdirectories are picked up. |
| `script` | path | Python script (default `th.py`) defining a programmatic suite.  See [Programmatic suites](#programmatic-suites). |
| `begin` | string | Shell command run before the suite (suite-level only). |
| `end` | string | Shell command run after the suite (suite-level only). |
| `build` | string | Shell command for `testHarness build` (suite-level only). |
| `clean` | string | Shell command for `testHarness clean` (suite-level only). |

String commands are split with `shlex`.  Lists are taken verbatim.

### Variable interpolation

Python `%(name)s` syntax is expanded in `command` and `args`:

| Variable | Value |
|---|---|
| `%(top)s` | the directory where testHarness was invoked |
| `%(suite-dir)s` | the suite's directory (e.g. `/path/to/JSONTests`) |
| `%(test-dir)s` | the case's run directory |
| `%(data-dir)s` | the case's `data/` directory |
| `%(expect-dir)s` | the case's `expect/` directory |

Any other key in the merged config is also available; this is how
custom config (e.g. `top`, `script`) can be referenced from commands.

## Checks

The default check is "standard" — diff `stdout`, `stderr`, and
`return` against `expect/`.  Override with a `checks` array in
`test.json`.  Each entry is either a string naming a check class, or
a list `[class, arg1, arg2, ...]`.

```json
{
  "command": "./compress in.txt out.bz2",
  "checks": ["standard",
             ["min_size", "out.bz2", 40],
             ["max_size", "out.bz2", 80]]
}
```

| Check | Form | Behavior |
|---|---|---|
| `standard` | `"standard"` | Diff stdout, stderr, return. |
| `file` | `["file", NAME, FILTER?]` | Diff a named file against `expect/NAME`.  Optional filter (see [Filters](#filters)). |
| `exists` | `["exists", NAME, SIZE?]` | The file must exist.  If SIZE is given, it must be exactly that many bytes. |
| `size` | `["size", NAME, N]` | File is exactly N bytes. |
| `min_size` | `["min_size", NAME, N]` | File is at least N bytes. |
| `max_size` | `["max_size", NAME, N]` | File is at most N bytes. |
| `pattern` | `["pattern", NAME, GLOB]` | Glob `GLOB` (with variable interpolation) and write the sorted list of matched files to NAME, then diff against `expect/NAME`.  Useful for verifying which files a test produces. |
| `db` | `["db", NAME, DB, ...]` | Run a SQL query on a SQLite database produced by the test, format as a text table, diff against `expect/NAME`.  See source for options (`sql`, `cols`, `table`, `orderBy`). |
| `command` | `["command", NAME, CMD]` | Run an external command, capture its output to NAME, diff against `expect/NAME`.  Useful for post-processing. |

When `checks` is set, the default standard check is **not** added
automatically — include `"standard"` explicitly if you still want it.

### Filters

Filters apply to `file` checks before diffing.  They transform each
line of the captured output.

| Filter | Form | Effect |
|---|---|---|
| match | `["match", REGEX]` | Keep only lines that match `REGEX`. |
| not_match | `["not_match", REGEX]` | Drop lines that match `REGEX`. |
| replace | `["replace", REGEX, REPLACEMENT]` | Substitute the first match in each line.  If `REGEX` has groups, `REPLACEMENT` is a list of per-group replacements. |

A bare regex string as the filter argument is shorthand for `match`:

```json
{"checks": [["file", "stdout", "^OK"]]}
```

Useful for normalizing variable output before diffing — e.g. replacing
timestamps with placeholders:

```json
{"checks": ["standard",
            ["file", "stdout",
             ["replace", "\\d{4}-\\d{2}-\\d{2}", "DATE"]]]}
```

## CLI commands

The general form is:

```sh
testHarness [options] [command] [target] [name=value...]
```

| Command | Argument | Action |
|---|---|---|
| `run` | target (optional) | Run a suite or case.  Default if no command given. |
| `init` | target | Capture current output as the expected output. |
| `diff` | target | Show diff between last run and expected output. |
| `view` | target | Print all expected and actual files for a case. |
| `reset` | target | Delete `expect/` files for a case. |
| `enable` | target | Remove the `disable` marker file. |
| `disable` | target | Create the `disable` marker file. |
| `setup` | target | Prepare the run dir without running the test. |
| `build` | target (optional) | Invoke the `build` config key as a shell command (or `build()` on a programmatic suite). |
| `clean` | target (optional) | Invoke the `clean` config key. |

A `target` is a suite name, case name, or a path like
`SuiteTests/CaseTest`.

`name=value` arguments after the command set entries in the top-level
config — useful for overriding `timeout`, `top`, etc. ad hoc.

## Flags

| Flag | Effect |
|---|---|
| `-I`, `--interactive` | On failure, prompt with (d)iff, (i)nit, (v)iew, (r)eset, (x)disable, (c)ontinue, (a)bort, (q)uit, (t)est. |
| `-f`, `--force-interactive` | Prompt even on passing tests. |
| `-v` | Verbose: print each command before running. |
| `-V`, `--valgrind` | Prefix each test command with `valgrind`. |
| `-P PREFIX`, `--command-prefix PREFIX` | Arbitrary command prefix (e.g. `-P 'timeout 5'`). |
| `--no-color` | Disable ANSI color in terminal output. |
| `-c FILE`, `--config FILE` | Load a Python configuration file (sets top-level config). |
| `-C DIR` | Change to `DIR` before running. |
| `-d`, `--dump` | Print the resolved top-level config and exit. |
| `-s`, `--save` | Keep all `run/` directories after the test (default: clean up passing tests). |
| `--save-failed` | Keep `run/` directories of failed tests only. |
| `--view-failed` | Print all files for failed tests. |
| `--view-all` | Print all files for every test. |
| `--diff-failed` | Show diff on failed tests automatically. |
| `--view-unfiltered` | Also display the unfiltered output alongside the filtered one in views. |
| `--build` | Run the `build` command for each suite before running tests. |
| `--json FILE` | Write a structured JSON log of the run. |
| `--junit FILE` | Write a JUnit XML report (for CI integration). |

## Disabling a test

Create an empty `disable` file in a case directory:

```sh
touch failingTests/BrokenTest/disable
```

Disabled tests are reported as DISABLED and don't affect the pass/fail
total.  Re-enable with:

```sh
./testHarness enable failingTests/BrokenTest
```

## Workflow tips

**Writing tests:**

- Build your program so it produces deterministic, line-oriented output.
- Run the test once with `init` to capture expected output, then read
  through `expect/stdout` carefully to confirm correctness before
  committing.
- Time-sensitive, random, or path-dependent output: normalize with a
  `replace` filter rather than hoping for stability.

**Debugging failures:**

- `--save-failed` keeps the failed run dirs so you can poke at them.
- `--diff-failed` auto-shows the diff inline; useful in CI logs.
- `-I` for interactive triage — diff, re-run, re-init in one session.
- `--json FILE` captures everything (commands, outputs, status) for
  post-mortem analysis.

**Updating expectations after intentional changes:**

- `init` is idempotent — re-running it re-captures expected output.
- If many tests need re-initialization at once, run with `-I`, hit
  `i` on each failure.  Or, less interactively: `./testHarness init
  <target>` per case.

**Performance:**

- Each test forks a process.  Hundreds of cases can be sub-second.
- For interactive use, `--diff-failed --save-failed` is a useful default.

## Programmatic suites

A suite can supply a Python file (default `th.py`) that defines a
`Suite` class.  The harness instantiates it with the `TestSuite`
object, letting the script add tests programmatically:

```python
# myTests/th.py
class Suite:
    def __init__(self, suite):
        for case in suite.config['cases'].split(','):
            test = suite.Test(case + 'Test', command = 'myprog ' + case)
            test.Test('Variant1', args = ['--mode', '1'])
            test.Test('Variant2', args = ['--mode', '2'])

    def begin(self): pass    # called before suite runs
    def end(self):   pass    # called after suite runs
    def build(self): pass    # invoked by `testHarness build <suite>`
    def clean(self): pass    # invoked by `testHarness clean <suite>`
```

Tests created this way can also have *sub-tests* sharing the
parent's setup.  Each sub-test gets its own status; the parent
passes only if all sub-tests pass.

The script is loaded with the harness's globals available
(`CheckFile`, `MatchFilter`, etc.), so the same primitives are
available as in `test.json`.

This mechanism is rarely needed; declarative `test.json` files cover
most cases.

## Output formats

**JSON** (`--json FILE`): nested object mirroring the suite tree with
status, reason, captured files, and a summary block.  Stable but
custom — read by humans or scripts, not by CI systems.

**JUnit XML** (`--junit FILE`): Apache Ant-flavor JUnit report.
Recognized by every major CI system (GitHub Actions, GitLab,
Jenkins, etc.) for rendering per-test pass/fail panels and inline
failure messages.
