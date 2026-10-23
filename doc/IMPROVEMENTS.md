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

**None of these workflows has run on GitHub yet.** Here `[x]` means that the
workflow is committed and was checked locally (the YAML parses, and its build,
test and packaging steps pass on trixie), not that it works on GitHub.

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
- [x] 3.14 [`a9c7f9d`](https://github.com/charlesfleche/lpd8editor/commit/a9c7f9da113f88841180d0b09da8417ad04f19c4) Startup: no exceptions escaping `MainWindow`, and a message box on
      fatal errors instead of `std::terminate`.
- [x] 3.15 [`41cc82d`](https://github.com/charlesfleche/lpd8editor/commit/41cc82dc843fde8f6c9df109f5d108ca88905d8b) Import and export: show errors to the user, add file filters, use
      `QFileInfo` for names, and wrap user-visible strings in `tr()`.
- [x] 3.16 [`230e729`](https://github.com/charlesfleche/lpd8editor/commit/230e72976efd8893218e2eaba90a306336f70930) Qt idioms: hold `Ui::MainWindow` in a `std::unique_ptr`, get the delegate's style
      from its widget, drop the noisy `Q_CHECK_PTR` calls, and use the
      multi-argument `QString::arg`.
- [x] 3.17 [`25df6fe`](https://github.com/charlesfleche/lpd8editor/commit/25df6fe25769da49bbd8afeb4fd314a409253568) Replace magic column numbers with named constants.

## Phase 4: architecture

The target design and the order of the steps are described in
`doc/ARCHITECTURE.md`. Read it before these commits.

- [x] 4.1 [`561e2e5`](https://github.com/charlesfleche/lpd8editor/commit/561e2e56418c30b205821614a79cff15253c71ab) Domain value types: `Program`, `Pad`, `Knob`, and a `DeviceProfile`
      (device slots and factory program; the control counts are fixed by the
      `Program` type). Its test compares only part of the factory program
      with the old SQL trigger: 4.2 compares all of it, byte for byte.
- [x] 4.2 [`691fb56`](https://github.com/charlesfleche/lpd8editor/commit/691fb56505a6aefe22d8e2a9dc2c905fdd52ca3a) Pure SysEx codec: `decodeProgram` and the encoders, with
      `std::expected` and round-trip tests. It replaces the byte parsing of
      `db.cpp`, which now reads and writes SysEx through it. The helpers in
      `lpd8_sysex.cpp` still serve the MIDI path and the file checks: 4.10
      moves those to the codec.
- [x] 4.3 [`226a7f7`](https://github.com/charlesfleche/lpd8editor/commit/226a7f77a56fcc37016c7799b5b7f3387b4f9426) `ProgramStore`: the program list in memory, saved as JSON with
      `QSaveFile` and a format version, with tests.
- [x] 4.4 [`cab90e8`](https://github.com/charlesfleche/lpd8editor/commit/cab90e86fd113eb59ed183dc27b0b58fc1fcb220) One-time migration from `db.sqlite3` to the JSON store. It must
      expect databases created before 2018 (v0.0.0 to v0.0.5), whose tables
      have no CHECK constraints and can hold any integer: report bad rows
      to the user, never wrap or drop them silently.

Items 4.6, 4.5 and 4.7a are done in that order, as written below. The views
can't show the JSON store while the edits, undo, MIDI and file paths still
write to SQLite: there would be two sources of truth. So the store-based
commands come first, then the models (used by nothing yet), then one commit
that switches everything at once.


- [x] 4.6 [`0fd036d`](https://github.com/charlesfleche/lpd8editor/commit/0fd036da32659896430b4ad443cfcfc83f42bedb) Undo commands over `ProgramStore`, by program id: set a program,
      rename, add, remove. Each holds ids and values, never rows or pointers.
      An undo whose program is gone marks itself obsolete instead of
      asserting. A command that changes nothing is not pushed. Repeated edits
      of one value merge (`mergeWith()`) only if the current
      `UpdateParameterCommand` does: keep today's behaviour. Undoing a removal
      puts the program back *at its row*, so the store gets an insert-at-row,
      whose signals carry that row. Tests: remove the middle program, undo,
      and the order is the same (same id at the same row), then redo and undo
      twice more without drift. Nothing uses the commands yet.
- [x] 4.5 [`3a994a4`](https://github.com/charlesfleche/lpd8editor/commit/3a994a4952945d02d14a83d58bd5cfac2d4bf1ae) New models: a programs list model and a controls table model for
      one program. They are driven only by `ProgramStore`'s signals, and their
      `setData()` pushes the commands of 4.6. They have exact column counts
      and readable, translatable headers (no `pc`, `cc`, `toggle`). Tests:
      `QAbstractItemModelTester` in Fatal mode on every model in every test,
      and changes made to the store directly (not through the model) that
      check the rows and the `dataChanged` ranges (one cell for a rename, the
      program's control cells for a program change). No window change yet.
- [x] 4.7a [`3824a69`](https://github.com/charlesfleche/lpd8editor/commit/3824a69b73697f40fe842d4be5c59a0c3af0bf8b) The switch, in one commit: the window, the undo stack, MIDI send and
      receive, and import/export move from SQLite to the store, the models of
      4.5 and the commands of 4.6.

      - Startup: `main()` creates the store, runs `migrateSqliteDatabase()`
        and loads the store. Until the real messages of 4.11: if the
        migration fails or the store can't be loaded, a message box names the
        file and the reason, says that nothing was changed, and says what the
        user can do (a newer format: use the newer lpd8editor; a damaged file:
        its path, to repair or move it), and the application quits. Starting
        empty instead would write the JSON file and strand the old programs.
        Tests: a refusal leaves both files byte-identical. A migration report
        (programs migrated, skipped and why, where the old database is) is
        shown in a message box.
      - Saving: after each change of the store (its `programAdded`,
        `programRemoved`, `programChanged` and `nameChanged` signals, so a
        change made without the undo stack is saved too), coalesced with a
        zero-millisecond single-shot timer so that many changes in one
        event-loop turn make one write. A failed save is shown to the user
        once, and remembered until a save succeeds again, not one dialog per
        change and not only a log line. The application also saves when it
        quits, on every way out, since the document doesn't save when it is
        destroyed: the Quit action, closing the window (`closeEvent`), and
        the session manager (`QGuiApplication::commitDataRequest` /
        `aboutToQuit`). Tests: N changes in one event-loop turn make one
        write, a direct change of the store saves once, a failing save is
        reported once, and `mainwindow_test` checks that closing the window
        leaves the last edit in the file.
      - No behaviour change: MIDI receive still applies a dump to the
        selected program; import and export keep their file format byte for
        byte (an export of the same program is identical before and after
        the switch, tested against a fixture); the `default/sysex` setting
        keeps working. A behaviour change gets an item of its own.
      - Selection: a rename, and the removal of another program, keep the
        selected program selected. Removing the selected program selects its
        neighbour, or nothing if the list is empty. Tested on the real window.
      - The undo stack: owned next to the store, declared after it so it is
        destroyed first, instead of the global `undoStack()` child of `qApp`
        (the first half of 4.11's 'App owns the stack'). The commands' header
        claims this order. Whoever owns the stack clears it on
        `programsAboutToBeReset`: commands hold ids, which only mean
        something for the content they were made against, and after a
        `load()` an id may name another program, which an undo would edit
        silently. Test: push an edit, `load()`, and the stack is empty, so
        undo does nothing.
      - `ProgramIdSelectionRestorer` and the substitute header views are
        deleted. The window is rendered offscreen (`QWidget::grab()`) before
        and after, and the visual change is described.

      It must also fix two symptoms of the tree of proxies (moved here from
      4.5, since they are in the window), found while
      investigating a warning in `mainwindow_test`:

      - `QAbstractItemView::setRootIndex failed: index must be from the
        currently set model`, twice when a program is selected. The pads and
        knobs tables have substitute horizontal headers whose model is the
        raw `QSqlTableModel` (for column names, since
        `ProgramsModel::headerData()` always returns the pads' headers).
        `QTableView::setRootIndex()` passes the index on to the headers,
        which reject it. Removing the substitute headers removes both
        warnings.
      - `ProgramsModel::columnCount()` returns 6 at every level, so the knobs
        view has a sixth, data-less column. It is not visible: the window,
        rendered offscreen with `QWidget::grab()`, shows exactly cc, low and
        high, because the substitute header has 5 sections. What users do
        see is the raw SQL column names as headers (`pc`, `cc`, `toggle`),
        which the new models should replace with readable, translatable ones.

      Tests: `mainwindow_test` runs with `QTest::failOnWarning()`, and checks
      the exact column counts of both views (4 visible fields for pads, 3 for
      knobs).

- [x] 4.7b Remove the SQLite schema, triggers, and `db.cpp`, `ProgramsModel`,
      the old undo commands and the storage helpers of `lpd8_sysex.cpp`.
      QtSql stays only for the migration. (Earlier text refers to this as
      "item 4.7".)
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

      If the programs file can't be read at startup (`ProgramStore::load()`
      fails: corrupt, or written by a newer version), `App` must tell the
      user why, and must not save over it. Setting the file aside
      (`ProgramStore::setAsideUnreadableFile()`, which renames and never
      deletes) happens only after asking the user. Until then the store
      refuses to save, so the user must also be told that changes can't be
      kept.

      `App` also runs the migration (`migrateSqliteDatabase()`, item 4.4) at
      startup, and must show its result at the first start after it ran:
      how many programs were migrated, each skipped program with the reason,
      and where the old database now is, because the skipped programs' data
      only lives there. The same report is saved next to the backup
      (`<backup>.txt`, written by the migration before the rename), so it
      can still be found if the application stops before the user reads
      it: the message names that file. If the old database could not be
      renamed, say so. If
      the migration fails (an unreadable database, for example one left with
      a hot journal by a crash), tell the user at each start, since it will
      be retried and fail the same way.

## Phase 5: packaging and documentation

- [ ] 5.1 Install a `.desktop` file, the icon, and AppStream metadata, and
      validate them.
- [ ] 5.2 Update `README.md` and `CLAUDE.md`: presets, tooling, supported
      platforms.
- [ ] 5.3 Release notes for the next release: the changes users can notice
      since v0.0.18 (the last tag). Each commit that changes behaviour adds
      a line below, so none is found later as a regression. Items before
      4.7a were not audited for this yet: that is part of this item.

      - Importing a program file whose slot byte (the 8th value) is not 1
        to 4 is refused as "not an LPD8 program" (since 691fb56, when the
        import started going through the codec). Files exported by
        lpd8editor always have slot 1, so only files from elsewhere are
        affected.
      - The same holds for the `default/sysex` setting, the template of new
        programs: one with such a slot is ignored, with a warning, and the
        factory program is used.
      - The pads and knobs headers are readable names ("Note", "Program
        change", "Control change", "Mode"; "Control change", "Low", "High")
        instead of the database's column names ("note", "pc", "cc",
        "toggle"; "cc", "low", "high").
      - The programs are kept in `programs.json` instead of `db.sqlite3`,
        which the first start migrates and keeps as
        `db.sqlite3.migrated`, with a report next to it (item 4.7a).
      - Quitting without an edit no longer writes the programs file.
      - Known quirk, not a change to announce: the pads and knobs tables are
        empty until a program is selected, so the first selection can make
        a window at its minimum width about 28 pixels wider, once (the
        tables count their scroll bars for one layout pass). The old tables
        always had rows.
- [ ] 5.4 Retire the migration from `db.sqlite3`, once no supported upgrade
      path needs it (a release or two after the switch to the JSON store):
      `migration.*`, its tests and `tests/sqlite_schema.h`, the `Qt::Sql`
      link, and the `libqt6sql6-sqlite` package dependency in
      `CMakeLists.txt` and `CONTRIBUTING.md`. The release notes say from
      which version an old database is no longer read.
