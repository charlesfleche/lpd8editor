# Contributing to lpd8editor

## Supported platforms

The code must build and pass its tests with **both GCC and Clang** on:

- Debian 13 "trixie": GCC 14, Clang 19, Qt 6.8, CMake 3.31. These are the
  oldest versions we support, so don't use anything newer.
- GitHub's `ubuntu-26.04` runner image.

## Building and testing

Dependencies on Debian or Ubuntu:

```bash
sudo apt install cmake ninja-build g++ clang libasound2-dev qt6-base-dev libqt6sql6-sqlite qt6-svg-plugins
# For the checks below:
sudo apt install clang-format clang-tidy cppcheck valgrind
```

Use the CMake presets (`cmake --list-presets` lists them). Each one builds into
`build/<preset>/`:

```bash
cmake --workflow --preset gcc-debug      # configure, build and test with GCC
cmake --workflow --preset clang-debug    # the same with Clang
```

To work on one configuration:

```bash
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug               # all tests
ctest --preset clang-debug -R db_test    # one test
./build/clang-debug/tests/db_test test_createProgram   # one test function
```

## Checks

Run these before sending a change. CI runs them too.

| Check | Command |
|---|---|
| Formatting | `cmake --build --preset clang-debug --target check-format` (fix with `--target format`) |
| clang-tidy | `cmake --build --preset clang-debug --target clang-tidy` |
| cppcheck | `cmake --build --preset gcc-debug --target cppcheck` |
| Sanitizers | `cmake --workflow --preset clang-asan` (or `gcc-asan`) |
| Valgrind | `cmake --preset valgrind && cmake --build --preset valgrind && ctest --test-dir build/valgrind -T memcheck` |
| Warnings as errors | configure with `-DLPD8EDITOR_WERROR=ON` |

A git pre-commit hook runs the fast checks (formatting of the staged files and
whitespace errors) at each commit. It is opt-in, once per clone:

```bash
git config core.hooksPath tools/git-hooks
```

Use the Clang presets for clang-tidy, which doesn't understand GCC-only
warning flags. Valgrind and the sanitizers need separate builds, because both
replace the memory allocator.

## Coding conventions

Most of the style is enforced by `.clang-format` and `.clang-tidy`. The rules
below are the ones a tool can't check, or that need their reason explained.

### Language

- C++23 without compiler extensions. Prefer the standard library and the
  language over macros: `constexpr` instead of `#define`, `std::array` instead
  of C arrays, `std::ranges` algorithms instead of hand-written loops over
  indexes.
- Put file-local functions, types and constants in an anonymous namespace.
  Without it, two files that each define a helper with the same name break the
  One Definition Rule. The one exception is an operator that must be found by
  argument-dependent lookup on a type from a C library, such as
  `operator==(snd_seq_addr_t, snd_seq_addr_t)`: make it `static` instead.
- Write `const` everywhere it applies, in west-const style (`const int x`).
- Mark single-argument constructors `explicit`.
- Make every narrowing conversion an explicit `static_cast`. The warnings
  (`-Wconversion`) make sure nothing narrows by accident, and MIDI code is full
  of int-to-7-bit-byte conversions. Qt counts with signed `qsizetype` and the
  standard library with unsigned `size_t`: use `std::ssize()` to bridge them.
- Never put code with side effects inside `Q_ASSERT`, which is removed in
  release builds.

### Naming

- Types: `UpperCamelCase`. Functions and variables: `lowerCamelCase`.
- Data members: `m_` prefix (`m_programs`). Some older members don't follow
  this yet. Rename them when you touch that code.
- File-local constants: `lower_snake_case` (`sysex_start`), as in the existing
  code.
- Header guards: `FILENAME_H`.

### Qt

- Ownership follows Qt's parent-child model: a `QObject` created with a parent
  is deleted by that parent. Anything else is owned by a `std::unique_ptr` or
  held by value. No bare `new` without an owner.
- Use the function-pointer `connect()` syntax. Connect a lambda with a
  **context object** (`connect(sender, &S::signal, context, [...]{})`), so the
  connection is broken when the context is destroyed, and list its captures
  explicitly (`[this, row]`, never `[=]` or `[&]`).
- `QObject` subclasses that define a destructor also say
  `Q_DISABLE_COPY_MOVE(ClassName)`.
- Wrap user-visible strings in `tr()`.
- Models must follow the `QAbstractItemModel` contract. Tests run them through
  `QAbstractItemModelTester`.

### Errors

- A failure that can happen at run time (a bad file, a missing device, a
  database error) is reported to the caller, and in the end to the user. The
  target is `std::expected<T, Error>`. Today the code still uses a mix of
  bools, exceptions and `qWarning()`, which the architecture items replace.
- Exceptions must never cross the Qt event loop (slots, lambdas, virtual
  functions called by Qt).
- `Q_ASSERT` is for programmer errors only, never for input validation.

### Tests

- Tests use QtTest and live in `tests/`, one executable per unit:
  `tests/<name>_test.cpp`, registered with `_add_test(<name>)` in
  `tests/CMakeLists.txt`.
- Use `QTEST_GUILESS_MAIN` unless the test needs widgets (`QTEST_MAIN`). Tests
  run with the offscreen platform, so they never need a display.
- Use data-driven tests (`<test>_data()` with `QFETCH`) for tables of cases.
- Every bug fix comes with a test that fails without the fix.

## Commits

- One topic per commit, small enough to review in one sitting. Mechanical
  changes (reformatting, renames, tool-generated fixes) go in their own commit.
- The subject line is imperative and under about 70 characters ("Fix the
  undo crash after deleting a program"). The body says *why*, and anything a
  reviewer should know.
- Each commit builds and passes its tests.
