# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

lpd8editor is a Linux-only Qt6 desktop editor for the original Akai LPD8 pad controller (Mk2 and Wireless are not supported). It talks to the device over the ALSA sequencer using SysEx messages. Akai publishes no SysEx spec; the reverse-engineered format lives in `doc/SYSEX.md`, so check there before changing anything in `lpd8codec.*`.

## Build and test

Dependencies (Debian/Ubuntu): `cmake ninja-build g++ clang libasound2-dev qt6-base-dev libqt6sql6-sqlite qt6-svg-plugins`, plus `clang-format clang-tidy cppcheck valgrind desktop-file-utils appstream` for the checks. Targets are Debian trixie (GCC 14, Clang 19, Qt 6.8, CMake 3.31: the oldest versions allowed) and GitHub's ubuntu-26.04 image. Code must build with both GCC and Clang.

Use the CMake presets (`cmake --list-presets`). Each builds into `build/<preset>/`:

```bash
cmake --workflow --preset gcc-debug        # configure + build + test (also clang-debug, clang-asan)
cmake --build --preset clang-debug         # build only
ctest --preset clang-debug -R programstore_test     # one test
./build/clang-debug/tests/programstore_test test_saveAndLoadRoundTrip   # one QtTest function
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

Add `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON` to treat warnings as errors (CI does). The warning flags are in `cmake/CompilerWarnings.cmake`, linked as the `lpd8editor_warnings` target. Packaging: `cmake --build <dir> --target package` (DEB + TGZ into `<dir>/packages`).

Tests live in `tests/` and are registered in `tests/CMakeLists.txt` with `_add_test(<name>)`, which builds `<name>_test.cpp` and links it against the `lpd8editor-lib` static library. Adding a test means creating `tests/<name>_test.cpp` and adding one `_add_test` line. Tests run with `QT_QPA_PLATFORM=offscreen`; widget tests use `QTest::failOnWarning()`, so the offscreen plugin's "propagateSizeHints" warning must be expected each time a window or box is shown. `alsaseqtransport_test` opens real ALSA sequencer clients and skips without `/dev/snd/seq`; every other MIDI test uses `FakeTransport` and `Lpd8Simulator` (`tests/`). The `.desktop` and AppStream files in `data/` are validated by tests that are skipped when `desktop-file-validate` or `appstreamcli` is missing (CI configures with `-DLPD8EDITOR_REQUIRE_VALIDATORS=ON`, which makes them fail instead). On a release tag, `metainfo_release_version` checks that the metainfo's newest `<release>` is the tagged version.

The version comes from `git describe` (tags like `vx.y.z`), or else from the source folder name (`lpd8editor-x.y.z`). It is written into the generated `app_info.h`.

## Architecture

`doc/ARCHITECTURE.md` describes the design and why; this is the map. Application code is in `src/`, tests are in `tests/`. Everything except `main.cpp` builds into `lpd8editor-lib`, so tests can link the same code the app uses.

- **`App`** (`app.*`) is the composition root. `main()` is `QApplication`, `App`, `start()`, `exec()`. `start()` runs the one-time migration and loads the programs, asking the user through `AppDialogs` (modal message boxes; a fake in `app_test`) when either fails. App then owns, in this order, the `ProgramsDocument`, the `MidiTransport`, the `Lpd8Device` and the `MainWindow`, so the window is destroyed first. It makes the final save (window close, `aboutToQuit`, `commitData()` with a `SessionEnd`). Its dependencies are injected through `App::Options` (paths, dialogs, transport factory, clock, migration).
- **Domain**: `Program`/`Pad`/`Knob` (`program.h`) are value types; `DeviceProfile` (`deviceprofile.*`) holds the device slots and the factory program. The codec (`lpd8codec.*`) is the only code that knows the SysEx byte layout: `decodeProgram`, `encodeGetProgram` and `encodeSendProgram` return `std::expected`; `encodeStoredProgram` returns a plain `QByteArray` and requires `program.isValid()` (storage only holds valid programs).
- **Storage**: `ProgramStore` (`programstore.*`) keeps the programs in memory with stable ids that are never reused, and saves them as one JSON file (`programs.json`, `defaultStorePath()`) with `QSaveFile`. Reading is strict; a failed `load()` returns a `LoadError` (CannotRead, Damaged, NewerFormat) and the store then refuses to save until the file is set aside. `ProgramsDocument` (`programsdocument.*`) holds the store and its `QUndoStack`, saves after each change (coalesced by a zero-ms timer, failures reported once through a handler), and offers `saveFinal()` for the final save. Program files (`.lpd8`) are read and written by `programfile.*`. `migration.*` moves the programs of the old SQLite `db.sqlite3` to the JSON file once, read-only, and is the only user of QtSql (scheduled for removal in item 5.4).
- **Models and undo**: `ProgramListModel` and `ControlsModel` (`programmodels.*`) are flat models driven only by the store's signals. Every user edit is an undo command from `storecommands.*` (set program, rename, add, remove), which holds program ids and values, never pointers; models' `setData()` push them. `MidiValueDelegate` edits values in the views.
- **MIDI**: `MidiTransport` (`miditransport.*`) is the interface (ports, connect, send SysEx, `sysexReceived`); `AlsaSeqTransport` implements it with the ALSA sequencer, behind a pimpl, reassembling SysEx with `SysexAssembler`. Its constructor throws when ALSA is unavailable, and `main()` then turns MIDI off. `MidiConnectionsModel` lists the ports for the toolbar's combo box. `Lpd8Device` (`lpd8device.*`) speaks the protocol: one request at a time, a 2 s timeout, and only answers to its own requests, from ports the application receives from, are accepted.
- **`MainWindow`** (`mainwindow.cpp` + `mainwindow.ui`, AUTOUIC) displays what App gives it and owns none of it. It keeps view state (the program a pending request is for), shows every runtime problem in one non-modal box (`showProblem()`), and asks App's close guard before closing.

The code targets C++23 (`target_compile_features(... cxx_std_23)` on the library, without GNU extensions). Qt 6.8 is the minimum, so there is no Qt 5 compatibility code: use Qt 6 APIs directly.

## Conventions

Coding conventions are in `CONTRIBUTING.md` (C++23, Qt idioms, naming, errors, tests, commits). `doc/IMPROVEMENTS.md` is the improvement checklist: one commit per item, each commit ticks its item and links the previous item to its commit.
