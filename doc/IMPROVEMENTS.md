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
- [x] 1.9 [`431ecdd`](https://github.com/charlesfleche/lpd8editor/commit/431ecdd57ba9d51246e32d36b772f7ee3a2135aa) Add `CMakePresets.json` with named, reproducible configurations: GCC
      or Clang, debug or release, plus sanitizers and valgrind.
- [x] 1.10 [`a1c4ed2`](https://github.com/charlesfleche/lpd8editor/commit/a1c4ed2ecfe1c41ec046205cff398b59061c1e08) Add `.editorconfig` and `.clang-format`, plus `format` and
      `check-format` targets.
- [x] 1.11 [`04af0e5`](https://github.com/charlesfleche/lpd8editor/commit/04af0e556a7eddc4a498d3bf2d409006fbf42727) Reformat the whole tree with clang-format, a mechanical commit only,
      and list it in `.git-blame-ignore-revs`.
- [x] 1.12a [`a7f67c0`](https://github.com/charlesfleche/lpd8editor/commit/a7f67c018ad6a7b0e5cfbb1f47a58a232ed71e43) Add `.clang-tidy`, an `LPD8EDITOR_CLANG_TIDY` option, and a
      `clang-tidy` target. This commit only makes the findings visible
      (about 300).
- [x] 1.12b [`7116c5e`](https://github.com/charlesfleche/lpd8editor/commit/7116c5ebc9ed80eddd4f6d5afd25b7924e28bca3) Apply clang-tidy's automatic modernization fixes (`override`,
      `auto`, `return {}`, default member initializers, range-for...).
- [x] 1.12c [`0803157`](https://github.com/charlesfleche/lpd8editor/commit/0803157a2599cc90eb8e43bee777515ae810fea5) Give file-local functions and constants internal linkage.
- [x] 1.12d [`8502bfa`](https://github.com/charlesfleche/lpd8editor/commit/8502bfa8e46893af7f1944e348650e7be265fbc7) Make const what never changes, and initialize every variable.
- [x] 1.12e [`d063017`](https://github.com/charlesfleche/lpd8editor/commit/d063017314270f4b8e5df96f07f369a914da79a6) Spell out lambda captures instead of `[=]`.
- [x] 1.12f [`e913a2e`](https://github.com/charlesfleche/lpd8editor/commit/e913a2e1b176abd72c015bf2a1534a488f1545ab) Name parameters, the same way in declarations and definitions.
- [x] 1.12g [`1975c4c`](https://github.com/charlesfleche/lpd8editor/commit/1975c4c15ee62a2b6a00c3ff73ff7c88adc66d26) Smaller readability and bugprone fixes, and `Q_DISABLE_COPY_MOVE`.
- [x] 1.12h [`3ce5cbb`](https://github.com/charlesfleche/lpd8editor/commit/3ce5cbb0129cd402d5fcf5f15ea4fbeb1e337e80) Use `std::array` for the SysEx byte tables.

      Findings deliberately left for later items, which rewrite that code:
      `goto` and macros in `db.cpp` (4.7), the macro in `lpd8_sysex.cpp`
      (4.2), `const_cast` in `ProgramsModel` (4.5), pointer arithmetic in
      `IOMidi` (3.5), and dead code and mutable globals (3.1).
- [x] 1.13a [`a692917`](https://github.com/charlesfleche/lpd8editor/commit/a692917f6fa0004b37f2db138b3d3fe1962cd779) Add a `cppcheck` target and a suppressions file. This commit only
      makes the findings visible.
- [x] 1.13b [`8ea3fe7`](https://github.com/charlesfleche/lpd8editor/commit/8ea3fe7114bc133ef92a1907de93b3c4ce77924a) Fix the cppcheck findings: `explicit` constructors, and an
      assert with a side effect. `alloca` (3.5) and the unused `paths`
      (3.1) are left for their own items.
- [x] 1.14 [`630461f`](https://github.com/charlesfleche/lpd8editor/commit/630461faf6c7c6542a6cfba39c151fe990f3acb4) Write the coding conventions in `CONTRIBUTING.md` and update
      `CLAUDE.md` to match.
- [x] 1.15 [`51fc83c`](https://github.com/charlesfleche/lpd8editor/commit/51fc83c3b13b184d9159047df78b751c3f391d65) Add an opt-in git pre-commit hook that runs the fast checks.

## Phase 2: CI/CD (committed now, tested together later)

- [x] 2.1 [`6eef68d`](https://github.com/charlesfleche/lpd8editor/commit/6eef68dee2d4cc91ea9320b5ed984eaf34cfc454) Test workflow: a {trixie container, ubuntu-26.04} × {gcc, clang}
      matrix, runs on pull requests, least-privilege `permissions`, actions
      pinned to a commit SHA, and `concurrency` to cancel superseded runs.
- [x] 2.2 [`87f3725`](https://github.com/charlesfleche/lpd8editor/commit/87f3725f49a21fa366c8181f8edf57712109f98b) Quality workflow with jobs for clang-format, clang-tidy, cppcheck,
      ASan/UBSan, and valgrind.
- [x] 2.3 [`7c5e4a2`](https://github.com/charlesfleche/lpd8editor/commit/7c5e4a2965be6ce44b79a8a632de9e87eceae010) Release workflow: packages for both targets, a `SHA256SUMS` file,
      least privilege, and pinned actions.
- [x] 2.4 [`11330c8`](https://github.com/charlesfleche/lpd8editor/commit/11330c873d075af6deb9db1ad9b039951a6a43c3) Dependabot keeps the pinned action SHAs up to date.

## Phase 3: small, self-contained fixes (smallest first)

- [x] 3.1 [`6b3d080`](https://github.com/charlesfleche/lpd8editor/commit/6b3d080042a6e3e17b96a943e7c014a3491bbc8f) Remove dead code: the empty `Lpd8editor` class,
      `readAndValidateChar` (it also contained an impossible condition),
      `defaultSysex()`, stray globals, and the unused `paths` in `main()`.
- [x] 3.2 [`fcc1d51`](https://github.com/charlesfleche/lpd8editor/commit/fcc1d5145c50d21169da377bbe36eba2d90e86d5) Remove the Qt 5 compatibility code (`QT_VERSION` branches,
      `static_cast` signal overloads, the `setObsolete` shim) and use `nullptr`.
- [x] 3.3 [`28fddb7`](https://github.com/charlesfleche/lpd8editor/commit/28fddb752865718f4d6afce7d206798f1f9572f0) `MidiConnectionsModel`: fix the off-by-one `dataChanged` range.
- [x] 3.4 [`ba36d2a`](https://github.com/charlesfleche/lpd8editor/commit/ba36d2a541b747441252aba8093cebaaddd3e34e) `IOMidi`: use `QCoreApplication`, because the downcasts to
      `QApplication`/`QGuiApplication` are undefined behaviour without a GUI
      app (UBSan reports them).
- [x] 3.5 [`d27ce5e`](https://github.com/charlesfleche/lpd8editor/commit/d27ce5ec3826d7a6029315a013207472fd8e45bd) `IOMidi`: no dangling `alloca` member, check the result of
      `snd_seq_event_input`, and use RAII so a throwing constructor doesn't
      leak the sequencer handle.
- [x] 3.6 [`2775f9b`](https://github.com/charlesfleche/lpd8editor/commit/2775f9bfdb0cdc58e570a46b5bb2a56bea9683ad) Give every lambda `connect` a context object.
- [x] 3.7 [`91e31ac`](https://github.com/charlesfleche/lpd8editor/commit/91e31aca58dddfd9bbb437c1db14c61afac9d59d) `createProgram` rolls back instead of committing on failure, and
      `fromSysex` reports failures. Add regression tests.
- [x] 3.8 [`64e1f24`](https://github.com/charlesfleche/lpd8editor/commit/64e1f24898f3d0d59117fba9b6532a274ccf01d8) `updateProgramFromSysex`: check bounds *before* dereferencing.
- [x] 3.9 [`9917b18`](https://github.com/charlesfleche/lpd8editor/commit/9917b185e368137d25b609c9c5c5f9897182e558) `initDb`: report query errors and create the schema inside one
      transaction.
- [x] 3.10 [`5f4409b`](https://github.com/charlesfleche/lpd8editor/commit/5f4409b0afed1aa5ad4b97df092814a7fb310b1b) `ProgramsModel`: respect the `QAbstractItemModel` contract for
      invalid indexes and `hasIndex`, checked by `QAbstractItemModelTester`
      in the tests.
- [x] 3.11 [`dd04921`](https://github.com/charlesfleche/lpd8editor/commit/dd04921bfbeccad2bb65e1809353e626418316f5) `ProgramsModel::setData`: reject invalid values, skip changes that
      change nothing, and honour the role.
- [x] 3.12 [`d14dbc5`](https://github.com/charlesfleche/lpd8editor/commit/d14dbc51bcbcb2523b0f08b044830abf1439b60c) SysEx validation: check the manufacturer and model bytes, and
      validate imported files before they reach storage.
- [x] 3.13 [`5d101db`](https://github.com/charlesfleche/lpd8editor/commit/5d101dbec67e6e4d4c69b585a8209d669435c606) Undo commands store identities, not model pointers. This fixes the
      heap-use-after-free seen with ASan; add a regression test.
- [x] 3.14 Startup: no exceptions escaping `MainWindow`, and a message box on
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
