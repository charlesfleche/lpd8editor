# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

lpd8editor is a Linux-only Qt6 desktop editor for the original Akai LPD8 pad controller (Mk2 and Wireless are not supported). It talks to the device over the ALSA sequencer using SysEx messages. Akai publishes no SysEx spec; the reverse-engineered format lives in `doc/SYSEX.md`, so check there before changing anything in `lpd8_sysex.*`.

## Build and test

Dependencies (Debian/Ubuntu): `cmake ninja-build g++ clang libasound2-dev qt6-base-dev qt6-svg-dev`, plus `clang-format clang-tidy cppcheck valgrind` for the checks. Targets are Debian trixie (GCC 14, Clang 19, Qt 6.8, CMake 3.31: the oldest versions allowed) and GitHub's ubuntu-26.04 image. Code must build with both GCC and Clang.

Use the CMake presets (`cmake --list-presets`). Each builds into `build/<preset>/`:

```bash
cmake --workflow --preset gcc-debug        # configure + build + test (also clang-debug, clang-asan)
cmake --build --preset clang-debug         # build only
ctest --preset clang-debug -R db_test      # one test
./build/clang-debug/tests/db_test test_createProgram   # one QtTest function
```

When building in a scratch directory instead, chain the test run after the build (`cmake --build ... && ctest ...`). Otherwise a failed build silently runs the previous binaries.

Checks, all expected to be clean (see `CONTRIBUTING.md` for details):

```bash
cmake --build --preset clang-debug --target check-format   # or 'format' to fix
cmake --build --preset clang-debug --target clang-tidy     # Clang presets only
cmake --build --preset gcc-debug --target cppcheck
cmake --workflow --preset clang-asan                       # ASan + UBSan
cmake --preset valgrind && cmake --build --preset valgrind && ctest --test-dir build/valgrind -T memcheck
```

Add `-DLPD8EDITOR_WERROR=ON` to treat warnings as errors (CI does). The warning flags are in `cmake/CompilerWarnings.cmake`, linked as the `lpd8editor_warnings` target. Packaging: `cmake --build <dir> --target package` (DEB + TGZ into `<dir>/packages`).

Tests live in `tests/` and are registered in `tests/CMakeLists.txt` with `_add_test(<name>)`, which builds `<name>_test.cpp` and links it against the `lpd8editor-lib` static library. Adding a test means creating `tests/<name>_test.cpp` and adding one `_add_test` line. `iomidi_test` opens a real ALSA sequencer client, so it needs `/dev/snd/seq` to be available.

The version comes from `git describe` (tags like `x.y.z`), or else from the source folder name (`lpd8editor-x.y.z`). It is written into the generated `app_info.h`.

## Architecture

Application code is in `src/`, tests are in `tests/`. Everything except `main.cpp` builds into `lpd8editor-lib`, so tests can link the same code the app uses.

- **Storage is SQLite, and it is the source of truth.** `db.cpp` has free functions (`createProgram`, `fromSysex`, `programSysex`, `deleteProgram`, ...) that run on the default `QSqlDatabase` connection. `initDb` creates the schema from the `.sql` files embedded through `sql.qrc`. Tables: `programs` (name, MIDI channel), `pads` (8 per program: note/pc/cc/toggle), `knobs` (8 per program: cc/low/high). SQL triggers insert the default pads and knobs when a program is created and delete them when it is removed, so C++ never inserts those rows itself. The app database is at `defaultDbPath()` (`db.sqlite3` in the app data dir). Tests use `:memory:` and call `initDb` in `init()`.
- **SysEx ⇄ DB conversion**: `lpd8_sysex.*` builds and parses the raw message layout (header, channel, 8×4 pad bytes, 8×3 knob bytes, footer). `db.cpp` maps that layout to and from table rows (`programSysex` serializes, `fromSysex` updates rows).
- **`ProgramsModel`** is a custom `QAbstractItemModel` tree over three `QSqlTableModel`s. The top level holds programs. Each program has two group children, pads and knobs, and each group's children are rows from a per-program `QSortFilterProxyModel` over the pads/knobs tables. Value columns use lookup tables (note names, channels, toggle) for display. `MidiValueDelegate` edits those values in the views.
- **Undo/redo**: every user mutation is a `QUndoCommand` in `commands.*` (create/delete program, update parameter, update from SysEx), pushed onto the global `undoStack()`. Mutations go through these commands rather than through the model directly.
- **MIDI I/O**: `IOMidi` wraps an ALSA sequencer client and port, polls its fds, and emits `eventReceived`. Its constructor throws `std::runtime_error` when ALSA is unavailable, and `MainWindow` then turns MIDI features off instead of failing. `MidiConnectionsModel` lists the ALSA clients and ports in the connection combo box and handles subscriptions. `SysexHandler` sends get/send program requests for device slots 1–4 and emits `programReceived(QByteArray)`.
- **`MainWindow`** (`mainwindow.cpp` + `mainwindow.ui`, AUTOUIC) connects all of this together. A received program becomes an `UpdateProgramFromSysexCommand` on the selected program. The "send to program N" actions serialize the selected program with `programSysex` and send it to device slot N.

The code targets C++23 (`target_compile_features(... cxx_std_23)` on the library, without GNU extensions). Some code still has Qt 5 compatibility idioms (for example `static_cast` signal overloads); they are being removed (see `doc/IMPROVEMENTS.md`).

## Conventions and work in progress

Coding conventions are in `CONTRIBUTING.md` (C++23, Qt idioms, naming, errors, tests, commits). `doc/IMPROVEMENTS.md` is the ongoing improvement checklist: one commit per item, each commit ticks its item and links the previous item to its commit. Some code described above (the SQLite storage, `ProgramsModel`'s tree of proxies, `IOMidi`) is scheduled to be replaced there.
