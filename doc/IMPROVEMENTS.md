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
      `IOMidi` (3.5), and dead code and mutable globals (3.1). All closed:
      the last one, `RETURN_IF_TYPE` in `lpd8_sysex.cpp`, went with that
      file in 4.10b, and clang-tidy has no deferred finding left.
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

- [x] 4.7b [`60731f4`](https://github.com/charlesfleche/lpd8editor/commit/60731f44b529a96a4a48ef0e1d41181f98158d2d) Remove the SQLite schema, triggers, and `db.cpp`, `ProgramsModel`,
      the old undo commands and the storage helpers of `lpd8_sysex.cpp`.
      QtSql stays only for the migration. (Earlier text refers to this as
      "item 4.7".)
Items 4.8a to 4.10b follow the lesson of 4.7: what can't coexist (two
owners of one ALSA client, two paths for one MIDI menu) switches in one
commit, after the new code is in place and tested on its own.

- [x] 4.8a [`11abd91`](https://github.com/charlesfleche/lpd8editor/commit/11abd91d2bfc0ccc2fe0f180f23fd7f4bd87ca07) A `MidiTransport` interface, a `SysexAssembler`, and a
      `FakeTransport` for the tests. Nothing uses them yet.

      - The interface carries messages, not events, and no ALSA type:
        `ports()` (an opaque `QString` id, a name, the connection
        directions), `connectTo(id)`, `disconnectAll()`, `connectedPort()`,
        `externallyManaged()` (connections made by aconnect or qjackctl
        count, as today), `sendSysex(message)`, and the signals
        `sysexReceived(message, sourcePortId)`, `portsChanged()` and
        `connectionChanged()`. The source port is in the signal so 4.10
        can check it without changing the interface.
      - Port ids (M1) are valid only until `portsChanged()` reports the
        port gone: ALSA client numbers change when a device is replugged
        (20, then 24). Nothing persists an id. The application doesn't
        remember a device today (no setting, no auto-connect: the
        connection state is read from the live subscriptions), so no stable
        key is needed yet. A feature that remembers one must use the client
        and port names, resolved again after each `portsChanged()`.
      - `sendSysex()` (M3) takes one complete F0…F7 message and refuses
        anything else. It returns false and sends nothing when no port is
        connected. Otherwise it sends to the port's subscribers, as today:
        whatever the port's subscriptions are, including ones made by
        other tools. The ALSA backend documents its size limit (4.8b).
      - `SysexAssembler` (M2) turns bytes from one source into complete
        messages, per the MIDI spec. F0 starts a message, and a new F0
        restarts it. F7 ends it. Real-time bytes (F8 to FF) inside a
        message are skipped and don't end it. Any other status byte (80 to
        F6) aborts the message. A message longer than a documented limit
        (a few KB; an LPD8 message is 66 bytes) is dropped, so an F0 with
        no F7 can't grow without bound, and that is reported once. A test
        row for each case.
      - `FakeTransport` (tests only): a port list the test edits, the sent
        messages recorded, `receive(message, fromPort)` to inject one. It
        is what the tests of `MidiConnectionsModel`, `SysexHandler`,
        `MainWindow`, the simulator (4.9) and `Lpd8Device` (4.10) use.
        Test (M1): the connected device disappears and comes back with a
        new id. The connection state follows, and the source of what it
        sends is its new id.
- [x] 4.8b [`4f423ec`](https://github.com/charlesfleche/lpd8editor/commit/4f423ecba92818b88066d954e64ca6a662de8fbf) The switch, in one commit: `AlsaSeqTransport` replaces `IOMidi`
      and the ALSA code of `MidiConnectionsModel`.

      - `IOMidi` becomes `AlsaSeqTransport` with `git mv`, so the history
        follows (shown with `git diff -M --stat`). It alone owns the
        sequencer handle and the socket notifiers, in the order of C1
        (notifiers destroyed before the handle closes). A pimpl keeps
        `asoundlib.h` out of every header, so `ALSA::ALSA` becomes a
        PRIVATE link of the library.
      - It reassembles received SysEx with `SysexAssembler`, per source.
        It documents ALSA's limit for one outgoing SysEx event (the
        client's output buffer, `snd_seq_get_output_buffer_size()`), and
        the test sends a message larger than 66 bytes.
      - `MidiConnectionsModel` becomes a plain list model over
        `MidiTransport` (same rows, 'Disconnect' row, properties and
        signals), and `SysexHandler` sends and receives through it.
      - `MainWindow(ProgramsDocument &, MidiTransport *)`: nullptr turns
        MIDI off, as when ALSA fails today. `main()` creates the transport
        before the window, so it outlives it (the first half of 4.11's
        'App owns the transport'). C2: `~MainWindow` deletes the objects
        that connect into its lambdas while `ui` exists. The teardown test
        fires the fake's signals during the destruction.
      - `mainwindow_test` uses the fake, so its MIDI tests no longer depend
        on the machine.
      - `alsaseqtransport_test` replaces `iomidi_test` and is the only test
        that needs `/dev/snd/seq`. Two transports: B appears in A's ports,
        A connects to B, a SysEx goes each way with the right source id,
        and the disconnection is signalled. Without a sequencer it skips
        with a message saying why.
      - CI (M4), a note for the first CI run (phase 2): try `sudo modprobe
        snd-seq` (and `snd-seq-dummy`) as a non-fatal step on the Ubuntu
        runner, so this test can run there. Print the skipped tests, and
        fail if their count changes unexpectedly, so a skip is never
        silent.
      - No behaviour change: a program dump from any client still replaces
        the selected program (that is 4.10).
- [x] 4.9 [`7f930e4`](https://github.com/charlesfleche/lpd8editor/commit/7f930e41d15af8edfda64cc7bd9f1a9fc3e3ceab) An LPD8 simulator over `FakeTransport`, answering like the device
      (get program, send program, four slots), to test the device protocol
      without hardware.
- [x] 4.10a [`02fea82`](https://github.com/charlesfleche/lpd8editor/commit/02fea8281c402d2373bad120f54a5ad39b840dd8) `Lpd8Device`, tested against the simulator: match each program
      reply to its request, with a timeout. A reply goes where it was
      requested. Nothing uses it yet.

      - One request in flight: the next one is queued, not sent (tested by
        counting what is sent while one is pending). Sends, which have no
        answer, go through the same queue, so nothing is sent between a get
        and its answer.
      - A timeout of 2 seconds, documented. On a timeout: one
        `requestFailed` signal, no program applied, and the device is no
        longer busy, so the window (4.10b) re-enables the action and shows
        one message.
      - An answer for another slot (the simulator's WrongSlot mode) is
        ignored, not applied, and the request still times out.
      - Unsolicited dumps, and dumps from a port the application doesn't
        receive from, are ignored (the security finding). The connections
        are read at that moment, never a cached id (M1). Test: after a
        replug under a new id, an answer from the old id is ignored, and the
        device's are accepted.
      - Connections made by another tool keep working (E1, choice (a)):
        aconnect, qjackctl or a JACK bridge make no single "connected port",
        and the MIDI actions are enabled on any connection, as before. So a
        request goes to whatever receives from the application, as today,
        and an answer is accepted from any port the application receives
        from. With the connection made in lpd8editor, that is exactly the
        device. Dropping such setups would surprise their users after an
        update; this keeps them, and is still far stricter than accepting a
        dump from anyone. Tests: a both-ways patch plus a MIDI monitor
        works, and a dump from the monitor or from an unpatched client is
        ignored; a monitor added during a request doesn't fail it.
      - Ignored messages (another client, nothing asked, another slot) are
        counted, for the tests, and logged under the `lpd8.device` logging
        category, at debug level. A user doesn't notice them, which is
        fine; `QT_LOGGING_RULES=lpd8.device.debug=true` shows them.
      - The device unplugged or disconnected during a request (nothing sends
        to the application any more): the request fails at once, not at the
        timeout, and the queued ones too. No crash: tested by destroying the
        simulator with a get pending.
      - The header says that the signals may come before the call that
        caused them returns (requestFailed() from inside requestProgram()
        when nothing is connected), and that handlers must not block.
      - No reentrancy window: a test answers from inside the send call (its
        own send hook on the fake, test-only: real backends never do this),
        and the answer is accepted, since the request's state is set before
        the message is sent (L1).
- [x] 4.10b [`b893d16`](https://github.com/charlesfleche/lpd8editor/commit/b893d16e27c33b341b89d63e9f2f1ef8ef1091c2) The switch of the MIDI menu to `Lpd8Device`, in one commit.
      `SysexHandler` and what is left of `lpd8_sysex.*` are deleted, and
      the 'program received' path goes through the device. This is a
      behaviour change, for the release notes (5.3): only answers to the
      application's own requests are applied.

      - The dump lands on the program that was selected when the user asked
        for it (E2), not on whatever is selected when it arrives, up to two
        seconds later: the target program id is captured with the request.
        If that program was deleted in the meantime, the dump is dropped,
        with the one message. Test: request, change the selection, the
        answer arrives, and only the original program changes. A fix, for
        5.3: until then, the dump went to the selection at arrival.
      - While a request is in flight, the Get and Send actions are disabled,
        and enabled again on `busyChanged(false)`.
      - A failure is shown without blocking: a non-modal box
        (`QMessageBox::open()`, one reusable box), never `exec()` inside a
        handler, since the device's signals can come from inside its request
        loop. Several failures at once (an unplug with requests queued) show
        one message, not one each.
      - The old `Q_UNIMPLEMENTED()` for "nothing selected" goes: Get is
        disabled with no selection, and the target is fixed at request time.
- [x] 4.11a [`ab7be77`](https://github.com/charlesfleche/lpd8editor/commit/ab7be7765aafaf5ecd39073c5e7906b2d0e735ad) The programs file's load error gets a kind (`LoadError`):
      `CannotRead` (permissions, I/O, not a regular file), `Damaged`, and
      `NewerFormat`, so the startup can offer what fits (4.11b): only a
      damaged file may be set aside. `setAsideUnreadableFile()` takes the
      time that names the file, so a test can make the rename fail. Both
      error paths are tested in ways that work for root too, since CI may
      run as root, which ignores permissions: a directory and a FIFO named
      `programs.json`, and a set-aside whose target name is taken. The store keeps the kind, and
      `setAsideUnreadableFile()` refuses unless the last load failed as
      `Damaged` (a directory, then a set-aside: refused, nothing renamed).
      A `LoadError` built without a kind is `CannotRead`, the strictest. The
      file is read with a bound (the limit plus one byte), not checked for
      size and then read whole. A `stat` that fails with anything but
      "not found" (`EACCES` on a folder above the file) is `CannotRead`, not
      a first start; its test is skipped as root, which can enter any
      folder. The read-error path itself (an I/O error mid-read) is not
      tested: it can't be provoked without a faulty device.
- [x] 4.11b [`817bed3`](https://github.com/charlesfleche/lpd8editor/commit/817bed3d4e22879cbc8fbd5d3fec4f1774381bf7) Composition root: an `App` object runs the startup and owns the
      document, the transport and the window, in that order (so they are
      destroyed window first). `main()` becomes: `QApplication`, `App`,
      `exec()`. `startup.*` folds into it. For a load failing right after
      a migration, the part that reports the migration (its report in the
      refusal, and no "nothing was changed") is kept word for word; the old
      generic advice ("use the newer version, or move it elsewhere") goes,
      replaced by the advice of each kind below. The save at quit
      (`aboutToQuit` and `commitDataRequest`, connected in `main()` today)
      moves into `App` too.

      The startup questions go through a `StartupDialogs` interface,
      implemented with modal message boxes, used only before the event
      loop, and faked in the tests. Quit is the default button and Escape
      everywhere. Apart from the migration itself, nothing is written unless
      the user picks an action that writes, and setting a file aside is the
      only one. `App` takes the clock (the time that names a set-aside file)
      as a parameter, so a test can pick the target name and take it in
      advance.

      - The migration fails: Quit or Retry. Never "start without
        migrating", which would write the programs file and strand the old
        programs. The question says what to do before retrying (close the
        other program using the database; for a hot journal, where the
        `-journal` file is). To tell a hot journal apart, the migration's
        error becomes structured: `migrateSqliteDatabase()` returns a
        `MigrationError` (a kind, `HotJournal` or `Other`, the message, and
        the journal's path) instead of a `QString`. Test: a database with a
        `-journal` file next to it fails as `HotJournal`, with that path.
      - The programs file can't be loaded: `Damaged` offers Quit or Set
        aside (rename, never delete, then say where the file went, and
        start with no programs). `NewerFormat` and `CannotRead` offer Quit
        only: use the newer lpd8editor, or check the file's permissions.
        A format above 1 that is not an integer (1.5), or that doesn't fit
        in an int (1e20), counts as `NewerFormat` on purpose: the
        conservative choice, Quit only, since such a file may come from a
        version that doesn't exist yet. If the set-aside fails (the rename is refused), the error
        is shown, and then Quit is the only choice.
        There is no read-only mode: quitting would have to ask whether to
        lose the changes, and the session manager may forbid asking
        (`commitDataRequest` without interaction), so it couldn't be made
        honest.
      - The migration report, if any, is shown after the window, with the
        window as its parent. It names the report file next to the backup.
      - Tests, each branch through `App`, with both files byte-identical
        after every Quit: a first start; a migration and its notice
        (parented to the window); a migration failure, Quit, then Retry
        that succeeds; a damaged file, Quit, then Set aside (renamed,
        content intact, started empty, and the next start is a first start
        that leaves the set-aside file untouched); a newer format (no
        set-aside offered); a directory (`CannotRead`); a failed set-aside
        (the target name taken: the error, then Quit only, both files
        intact); and a load failing after a migration.
- [x] 4.11c [`dee562e`](https://github.com/charlesfleche/lpd8editor/commit/dee562e9406b3633edb7f6d1ca542949a88b03e7) `App` owns the device too, and the window only displays what it
      is given: `MainWindow(document, transport, device)` uses them and owns
      none. The selection-bound logic stays in the window, as view state
      (the target program of a request, the actions off while busy).

      - `App`'s members are, in order: the document, the transport, the
        device (it uses the transport), the window (it uses the device), so
        they are destroyed window first. `App`'s class comment says that
        the sanitizers catch a wrong order (the `clang-asan` preset, which
        also runs UBSan). `~App` clears the document's save-failure
        handler before the window goes, since the handler shows problems
        in the window.
      - The document's save-failure handler moves to `App`. Every runtime
        problem, MIDI or save, goes to the window's one non-modal box
        (`MainWindow::showProblem()`). A problem that comes while the box
        is shown is added to its text, unless it is already there: none is
        lost, and none stacks into a pile of boxes. Test: a MIDI failure,
        then a save failure while the box is up, give one box with both.
      - The final save is not a timer save: `App` makes it itself, through
        the document's final save, which returns the error instead of
        calling the failure handler, and `closeEvent` no longer saves on
        its own (it asks `App`, through a close guard the window is given).
        - When the window closes: a failed save asks "Close without saving"
          or "Cancel". Cancel ignores the close event: the window stays
          open, and the changes stay, to be saved again. Close without
          saving closes, and quitting then doesn't ask again.
        - At `aboutToQuit`, the application is ending and can't be stopped:
          a modal message (no parent window) is all that is possible.
        - Tests: a failing close with Cancel (window still open, the change
          still there), then with Close without saving; a failing
          `aboutToQuit` shows one modal message.
      - A save failing while the session manager ends the session (logging
        out): with no interaction allowed, it is logged with `qCritical`,
        and nothing is shown. With interaction allowed, it is asked for
        (`SessionEnd::allowsInteraction()`), shown modally, then the session
        is released. Tested through the session interface of `App::commitData()`
        and the dialog fake.
      - `StartupDialogs` becomes `AppDialogs`: its modal questions are used
        before the event loop runs, and when the window closes or the
        application quits, the moments when nothing non-modal can be shown.
      - The device now outlives the window, so `~MainWindow` disconnects
        it in its body, and the teardown test fires its signals too.
      - `doc/ARCHITECTURE.md`'s App section matches the code. (The line in
        5.3 about the startup's choices was added by 4.11b, with the
        change.) One more line in 5.3: closing the window when the
        programs can't be saved asks before losing the changes.

## Phase 5: packaging and documentation

- [x] 5.1 [`bd496d7`](https://github.com/charlesfleche/lpd8editor/commit/bd496d738c86df989590109bf5b6099cb075fe0a) Install a `.desktop` file, the icon, and AppStream metadata, and
      validate them. They are in `data/`, named after the application id
      `io.github.charlesfleche.lpd8editor`, installed by CMake (and so in the
      DEB), and checked by `desktop-file-validate` and `appstreamcli validate
      --no-net` as tests, skipped when the tool is missing.
- [x] 5.2 [`980785d`](https://github.com/charlesfleche/lpd8editor/commit/980785dcb1d229978cf9e5a09aa3336c7b748258) Update `README.md` and `CLAUDE.md`: presets, tooling, supported
      platforms. `CLAUDE.md`'s architecture map describes `App`, the store
      and document, the models, the transport and the device, and the
      warnings-as-errors flag is `CMAKE_COMPILE_WARNING_AS_ERROR`.
- [x] 5.3 [`8df1620`](https://github.com/charlesfleche/lpd8editor/commit/8df162047eb5835e1107d76fced4507cfdf26897) Release notes for the next release: the changes users can notice
      since v0.0.18 (the last tag). Each commit that changes behaviour adds
      a line below, so none is found later as a regression. The list below
      is the source; `doc/RELEASE-NOTES.md` is the same for users, grouped
      by topic, with a marked place for the security fixes of the separate
      security branch. Items before 4.7a were audited when this item was
      done: their lines are the first ones below, marked with the item.

      Before tagging the release:

      - Retake `doc/screenshot.png` on a real desktop: it shows the old
        interface (the database's column names as headers). The AppStream
        metadata points to it.
      - Add the release to `data/io.github.charlesfleche.lpd8editor.metainfo.xml`
        (`<release version="x.y.z" date="YYYY-MM-DD"/>`, newest first), and
        move the "Next release" notes of `doc/RELEASE-NOTES.md` under its
        number. On the tag, the `metainfo_release_version` test fails if the
        newest `<release>` isn't the tagged version, so the release workflow
        stops before publishing.

      - Fixed a crash: edit a pad, delete the program, undo twice (3.13).
      - Editing a value without changing it adds no undo step (3.11, kept
        by 4.6).
      - A failed import no longer leaves a half-made program (3.7), and an
        import of a file that is not an LPD8 program is refused with a
        message (3.12, 3.15).
      - Import and export show their errors instead of failing silently,
        and a failed export never leaves a truncated file (3.15).
      - The file dialogs filter LPD8 programs (`*.lpd8`), export suggests
        `<name>.lpd8`, and an imported program is named after the file
        without its extension (3.15). Before, the whole file name.
      - A dump from another manufacturer's device with the same opcode
        bytes is no longer taken for an LPD8 program (3.12).
      - When the data folder or database can't be opened, a message says
        so instead of an abort without a word (3.14; replaced by the
        startup choices of 4.11b).
      - A program file with more than 66 values (two programs back to
        back, for example) is refused; before, its first program was
        imported without a word (41cc82d).
      - A program message one byte short is refused; before, it was
        accepted, with the last knob value read as 0 (64e1f24, 3.8).
      - Cosmetic: the note D sharp is written D♯, like the other sharps,
        instead of D# (f34be32).
      - For packagers: C++23 (GCC 14, Clang 19), CMake 3.25 and Qt 6.8
        are required (1.2, 1.3); Git and Qt LinguistTools are optional
        (1.3). Release packages are built per distribution, with
        `SHA256SUMS` (2.3).

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
      - Only the answer to a request of the application is applied ('Get
        program N'), from a port connected to it. Before, any program dump
        that arrived, from any client and whether asked for or not,
        overwrote the selected program: the security finding of the
        review. Connections made with aconnect, qjackctl or a JACK bridge
        keep working.
      - The answer goes to the program that was selected when 'Get program
        N' was chosen, even if another one is selected by the time it
        arrives. Before, it went to the selection at arrival (a fix).
      - While the LPD8 is asked for a program, the MIDI actions are off. A
        device that doesn't answer within 2 seconds, or that is unplugged
        meanwhile, is reported in one message that doesn't block the
        window.
      - Devices plugged in or out are seen after using the connection menu
        too. Until the fix, connecting or choosing 'Disconnect' also
        dropped the subscription to ALSA's announcements, so the list
        stopped following hotplug (checked against 3824a69).
      - 'Send to program N' and 'Get program N' send nothing when nothing
        receives from the application's port; before, the message went to
        no one, with the same effect. The menu is disabled then
        anyway, unless another tool connected one way only.
      - When the programs can't be opened at startup, the message offers
        what fits: Retry after a failed migration (with how to repair a
        database a crash left half-written), Set aside for a damaged
        programs file (renamed, never deleted, then lpd8editor starts with
        no programs), and only Quit for a file from a newer version or one
        that can't be read. Before, every case quit (item 4.11b).
      - When the programs can't be saved, closing the window asks whether
        to close without saving, or to cancel and keep the window open.
        Before, the failure was shown and the window closed, losing the
        changes. A save that fails while editing is shown in a box that
        doesn't block the window, with any MIDI problem of the moment,
        instead of a modal dialog (item 4.11c).
      - lpd8editor appears in the desktop's application menu, with its icon,
        and in software centers (item 5.1). Before, the package installed
        only the program.
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
