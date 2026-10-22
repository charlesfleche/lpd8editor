# Improvement checklist

This file tracks the work that came out of the code review of lpd8editor. There
is one commit per item, and items are done in order: developer tooling first,
then CI/CD, then the smallest fixes, then the architecture rework.

How to read it:

- `[x]` means the item is done. The commit that does the work also ticks its
  own box.
- A commit cannot contain its own hash, so **the link to the commit that did an
  item is added by the next commit**. The links point to GitHub and work once
  the branch is pushed. Locally, use `git show <hash>`.
- Each item says *why* it matters, so you can read the commit with that in mind.

Supported targets: **Debian 13 "trixie"** (GCC 14, Clang 19, Qt 6.8, CMake 3.31)
and **GitHub's `ubuntu-26.04` image**. Everything here must build with both GCC
and Clang on both targets, so trixie sets the oldest versions we can rely on.

Decisions made before starting:

- MIDI stays on the ALSA sequencer, behind a `MidiTransport` interface.
  PipeWire bridges ALSA sequencer clients automatically, and JACK sees them
  through pipewire-jack or a2jmidid. Pulse has no MIDI.
- Program storage moves from SQLite to a JSON file written atomically with
  `QSaveFile`, with a one-time migration from the existing `db.sqlite3`.

## Phase 1: build system and developer tooling

- [x] 1.1 [`e637ae3`](https://github.com/charlesfleche/lpd8editor/commit/e637ae3ecceae84be80c4a69fa7ca9d0ea81efcc) Make the SQL-backed tests run on recent Qt (`QTEST_GUILESS_MAIN`).
      *Why:* 3 of 7 tests segfault on Qt 6.8, and no tooling can be checked
      while the tests are red.
- [x] 1.2 [`d008dfe`](https://github.com/charlesfleche/lpd8editor/commit/d008dfe07bdaefdfa8a427130aa9d0637f45220f) Raise the CMake minimum to 3.25 and the language to C++23, set per
      target. *Why:* Qt 6 needs C++17 anyway, and C++23 gives us
      `std::expected` for error handling. GCC 14 and Clang 19 support it.
- [x] 1.3 [`445e281`](https://github.com/charlesfleche/lpd8editor/commit/445e2818ebfa67a3191bad4fb1c4d0cae5facc15) Drop build dependencies we don't need: Git becomes optional, and
      LinguistTools is no longer required. *Why:* tarball builds failed without
      git, and translations were removed.
- [x] 1.4 [`96eb864`](https://github.com/charlesfleche/lpd8editor/commit/96eb8646ab3e154833b581f61efb0f60ad0111db) Move the tests to `tests/`. A pure rename, so it is easy to review.
- [x] 1.5 [`6478283`](https://github.com/charlesfleche/lpd8editor/commit/6478283031774ef1e1d59985221771d31ac81728) Build tests only when `BUILD_TESTING` is on, and run them headless
      (`QT_QPA_PLATFORM=offscreen`).
- [x] 1.6a [`a1785a0`](https://github.com/charlesfleche/lpd8editor/commit/a1785a0ca958814d4d9441441c618d506182e338) Add one shared compiler-warnings target for GCC and Clang, with an
      `LPD8EDITOR_WERROR` option. This commit only makes the warnings visible.
- [x] 1.6b [`a9ef1d0`](https://github.com/charlesfleche/lpd8editor/commit/a9ef1d0fbffddac7c8f804b32205d5d60768e451) Fix the about 50 warnings it shows, so `-Werror` builds pass.
- [x] 1.7 [`2f63718`](https://github.com/charlesfleche/lpd8editor/commit/2f637187bdb468b9222bde7c8c8fa362c8e2b20d) Add an `LPD8EDITOR_SANITIZERS` option for AddressSanitizer and
      UndefinedBehaviorSanitizer.
- [x] 1.8 [`f6ad9a7`](https://github.com/charlesfleche/lpd8editor/commit/f6ad9a72e4b246bcfe7c8373a9d7826290217baa) Run the test suite under valgrind memcheck with `ctest -T memcheck`,
      with a commented suppressions file.
- [x] 1.9 Add `CMakePresets.json` with named, reproducible configurations: GCC
      or Clang, debug or release, plus sanitizers and valgrind.
- [ ] 1.10 Add `.editorconfig` and `.clang-format`, plus `format` and
      `check-format` targets.
- [ ] 1.11 Reformat the whole tree with clang-format, a mechanical commit only,
      and list it in `.git-blame-ignore-revs`.
- [ ] 1.12 Add `.clang-tidy` and an `LPD8EDITOR_CLANG_TIDY` option, and fix
      (or explicitly justify) the findings.
- [ ] 1.13 Add a cppcheck configuration and a `cppcheck` target.
- [ ] 1.14 Write the coding conventions in `CONTRIBUTING.md` and update
      `CLAUDE.md` to match.
- [ ] 1.15 Add an opt-in git pre-commit hook that runs the fast checks.

## Phase 2: CI/CD (committed now, tested together later)

- [ ] 2.1 Test workflow: a {trixie container, ubuntu-26.04} × {gcc, clang}
      matrix, runs on pull requests, least-privilege `permissions`, actions
      pinned to a commit SHA, and `concurrency` to cancel superseded runs.
- [ ] 2.2 Quality workflow with jobs for clang-format, clang-tidy, cppcheck,
      ASan/UBSan, and valgrind.
- [ ] 2.3 Release workflow: packages for both targets, a `SHA256SUMS` file,
      least privilege, and pinned actions.
- [ ] 2.4 Dependabot keeps the pinned action SHAs up to date.

## Phase 3: small, self-contained fixes (smallest first)

- [ ] 3.1 Remove dead code: the empty `Lpd8editor` class,
      `readAndValidateChar` (it also contained an impossible condition),
      `defaultSysex()`, stray globals, and the unused `paths` in `main()`.
- [ ] 3.2 Remove the Qt 5 compatibility code (`QT_VERSION` branches,
      `static_cast` signal overloads, the `setObsolete` shim) and use `nullptr`.
- [ ] 3.3 `MidiConnectionsModel`: fix the off-by-one `dataChanged` range.
- [ ] 3.4 `IOMidi`: use `QCoreApplication`, because the downcasts to
      `QApplication`/`QGuiApplication` are undefined behaviour without a GUI
      app (UBSan reports them).
- [ ] 3.5 `IOMidi`: no dangling `alloca` member, check the result of
      `snd_seq_event_input`, and use RAII so a throwing constructor doesn't
      leak the sequencer handle.
- [ ] 3.6 Give every lambda `connect` a context object.
- [ ] 3.7 `createProgram` rolls back instead of committing on failure, and
      `fromSysex` reports failures. Add regression tests.
- [ ] 3.8 `updateProgramFromSysex`: check bounds *before* dereferencing.
- [ ] 3.9 `initDb`: report query errors and create the schema inside one
      transaction.
- [ ] 3.10 `ProgramsModel`: respect the `QAbstractItemModel` contract for
      invalid indexes and `hasIndex`, checked by `QAbstractItemModelTester`
      in the tests.
- [ ] 3.11 `ProgramsModel::setData`: reject invalid values, skip changes that
      change nothing, and honour the role.
- [ ] 3.12 SysEx validation: check the manufacturer and model bytes, and
      validate imported files before they reach storage.
- [ ] 3.13 Undo commands store identities, not model pointers. This fixes the
      heap-use-after-free seen with ASan; add a regression test.
- [ ] 3.14 Startup: no exceptions escaping `MainWindow`, and a message box on
      fatal errors instead of `std::terminate`.
- [ ] 3.15 Import and export: show errors to the user, add file filters, use
      `QFileInfo` for names, and wrap user-visible strings in `tr()`.
- [ ] 3.16 Qt idioms: hold `Ui::MainWindow` by value, get the delegate's style
      from its widget, drop the noisy `Q_CHECK_PTR` calls, and use the
      multi-argument `QString::arg`.
- [ ] 3.17 Replace magic column numbers with named constants.

## Phase 4: architecture

- [ ] 4.1 Domain value types: `Program`, `Pad`, `Knob`, and a `DeviceProfile`
      (control counts, device slots, default values).
- [ ] 4.2 Pure SysEx codec: `decode` and `encode` with `std::expected` and
      round-trip tests. It replaces the parsing spread across
      `lpd8_sysex.cpp` and `db.cpp`.
- [ ] 4.3 `ProgramStore`: the program list in memory, saved as JSON with
      `QSaveFile` and a format version, with tests.
- [ ] 4.4 One-time migration from `db.sqlite3` to the JSON store.
- [ ] 4.5 New models: a programs list model and a controls table model for the
      selected program. They replace the tree of proxies and are checked with
      `QAbstractItemModelTester`.
- [ ] 4.6 Undo commands work on `ProgramStore` by program id.
- [ ] 4.7 Remove the SQLite schema, triggers, and `db.cpp`. QtSql stays only for
      the migration.
- [ ] 4.8 A `MidiTransport` interface with an `AlsaSeqTransport`
      implementation: SysEx in and out as `QByteArray`, a port list, and
      hotplug. It replaces `IOMidi` and the ALSA code in
      `MidiConnectionsModel`.
- [ ] 4.9 A fake transport plus an LPD8 simulator, to test the device protocol
      without hardware.
- [ ] 4.10 `Lpd8Device`: match each program reply to its request, with a
      timeout. A reply goes where it was requested, and unsolicited dumps from
      other local clients are ignored (the security finding).
- [ ] 4.11 Composition root: an `App` object owns and wires the store, the
      device, and the window, so `MainWindow` only displays things.

## Phase 5: packaging and documentation

- [ ] 5.1 Install a `.desktop` file, the icon, and AppStream metadata, and
      validate them.
- [ ] 5.2 Update `README.md` and `CLAUDE.md`: presets, tooling, supported
      platforms.
