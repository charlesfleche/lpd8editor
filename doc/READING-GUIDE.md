# Reading guide for the `modernize` branch

This guide walks through the 74 commits of `modernize`, from `master` to the
security work. It is meant to be read next to the code. The order below is the
order I recommend, which is not always history order.

It is a personal reading aid: it is not committed, and its hashes are only
valid for the current history. If `modernize` is rewritten again, they change.

## How to read a commit

```bash
git show --stat <hash>      # the message, then the files it touches
git show <hash>             # the whole change
git show <hash> -- src/     # only one folder, to skip docs and tests
git log --reverse --oneline master..modernize    # the whole list
```

- **Subjects** start with the checklist item (`4.10a: ...`), so
  `doc/IMPROVEMENTS.md` tells you what each item was meant to do and why.
- **Messages of grouped commits** first list their parts, then give each
  part's own explanation under a `## ` heading. Read those: they hold the
  reasoning, often with what was measured or tried.
- **Tests first.** For a bug fix, read the test before the code: it is the
  shortest description of the failure.

## The story in five steps

1. **Phases 1 and 2** make changes safe: warnings, sanitizers, formatting,
   static analysis, CI.
2. **Phase 3** fixes bugs in the old code, smallest first.
3. **A review round** after 4.1 fixes what phases 1 to 3 got wrong.
4. **Phase 4** replaces the architecture: SQLite gives way to a JSON store,
   `IOMidi` and `SysexHandler` to a transport and a device, and `App` ties
   everything together.
5. **Security** is reviewed last, because the rewrite had undone some of the
   first security fixes.

## Suggested reading order

| # | Read | Time |
|---|---|---|
| 1 | The plan: `198d72a` | Short |
| 2 | The final design: `doc/ARCHITECTURE.md`, `CLAUDE.md` | Short |
| 3 | Phase 4, storage: 4.1 to 4.7b | Long |
| 4 | Phase 4, MIDI: 4.8a to 4.10b | Long |
| 5 | Phase 4, `App`: 4.11a to 4.11c | Medium |
| 6 | Security: `doc/SECURITY.md`, then the `Security:` commits | Long |
| 7 | Review fixes: `f34be32`, `fe80bcd` | Medium |
| 8 | Phases 1 to 3, whenever you are curious | As needed |

Starting from the design means you know where the code ends up before seeing
how it got there. Much of phase 3 fixes code that phase 4 later deleted, so it
matters more for its lessons than for the code that remains.

## Phase 4: storage

| Item | Commit | Subject | Look for |
|---|---|---|---|
| 4.1 | `561e2e5` | Domain types | Plain value types. The program's name stays out of `Program`, because the device doesn't store names. |
| 4.2 | `691fb56` | SysEx codec | Pure functions, no I/O: the only code that knows the byte layout, and easy to test. |
| 4.3 | `226a7f7` | `ProgramStore` | Programs in memory, saved as JSON with `QSaveFile`, an atomic write. |
| 4.4 | `cab90e8` | Migration | Run once, from a read-only database, and the old file is kept. Skipped programs are reported. |
| 4.6 | `0fd036d` | Undo commands | They hold program ids, not pointers. Done before 4.5 on purpose; the checklist explains why. |
| 4.5 | `3a994a4` | Models | Flat models driven by the store's signals. |
| 4.7a | `3824a69` | The switch | Everything that can't coexist changes in one commit: two sources of truth would be worse. |
| 4.7b | `60731f4` | Removal | The old code goes only after the switch. |

**The technique to take away:** build the new code next to the old, test it
while nothing uses it, switch in one commit, then delete. 4.7, 4.8 and 4.10
all follow this shape.

## Phase 4: MIDI

| Item | Commit | Subject | Look for |
|---|---|---|---|
| 4.8a | `11abd91` | `MidiTransport`, `SysexAssembler`, a fake | An interface in front of ALSA, so the tests need no hardware. |
| 4.8b | `4f423ec` | The switch | `IOMidi` is renamed first, on its own, so history follows the file. |
| 4.9 | `7f930e4` | LPD8 simulator | A fake device that answers like the real one. |
| 4.10a | `02fea82` | `Lpd8Device` | One request at a time, a timeout, and only answers to its own requests are accepted. |
| 4.10b | `b893d16` | The MIDI menu | The old code goes. The reply goes to the program selected when the request was made. |

## Phase 4: `App`

| Item | Commit | Subject | Look for |
|---|---|---|---|
| 4.11a | `ab7be77` | Load error kinds | Each failure gets its own kind, because each one calls for a different answer. The safety rule lives in the store, not the dialog. |
| 4.11b | `817bed3` | `App` | The composition root, dependencies injected through `App::Options`, and `AppDialogs` faked in tests. Quit is always the default. |
| 4.11c | `dee562e` | Final save | Three moments with three answers: closing can be cancelled, quitting can only inform, and logout may forbid dialogs. |

**Worth studying closely:**
- **Destruction order:** C++ destroys members in reverse order of declaration,
  so `App` declares the window last and it goes first.
- **Testing what can't be built:** `QSessionManager` can't be created in a
  test, so a small `SessionEnd` interface stands in front of it.
- **Testing a modal dialog:** a 0 ms timer fires inside the dialog's own
  event loop, and the test presses the keys from there.
- **Rules across a sequence:** at logout the save, the close and the quit run
  one after another, and "no dialogs" has to hold across all three.

## Security

Start with `doc/SECURITY.md`: the threat model says who can reach what. Then:

| Commit | Topic | Look for |
|---|---|---|
| `cab5c5a` | MIDI events from other programs | The most important one. A SysEx event can carry a pointer into the sender's memory. Trust only what the kernel sets. |
| `e4a5f8d` | Files | FIFOs that hang, numbers that wrap around, names that escape a folder, and files too large to load again. |
| `321f902` | Plain text | `QMessageBox` guesses whether text is HTML. A test now fails on the forbidden calls, because the bug came back once already. |
| `8f9e42a` | Fuzzing | Fuzzers check properties, not only crashes. The migration's is checked against SQLite's own `typeof()`. |
| `de7ddc4` | Hardening and CI | A test reads the built binary to check the flags really took effect. |
| `fb313da` | The write-up | The accepted risks: what was left unfixed, and why. |

`doc/SECURITY.md` cites fixes by their original subject. Those subjects are
now inside the grouped commits, so search for them:

```bash
git log --grep='Drop MIDI events' --oneline modernize
```

## Review fixes

Three commits sit right after 4.1. They hold the 18 review fixes that could
not be folded into their item without conflicts.

| Commit | Content |
|---|---|
| `fe80bcd` | Build and tooling: warnings-as-errors switch, ALSA imported target, valgrind time limits, tool versions, clang-tidy filters. Fixes for 1.2, 1.6a, 1.8, 1.10 and 1.12a. |
| `f34be32` | Code: notifier lifetime in `IOMidi`, updates of missing programs, note names, a header split, the database connection, the undo stack in tests. Fixes for 3.5, 3.8, 3.11, 3.13, 3.14 and 3.16. |
| `f33da4f` | The storage format in `doc/SYSEX.md`. Separate because `.git-blame-ignore-revs` names it: its line-ending change rewrote the file, and blame must skip that without skipping real code. |

These are good reading: each message says what went wrong, how it was found,
and how the fix was checked.

## Phases 1 to 3

**Phase 1, tooling**

| Item | Commit | Subject |
|---|---|---|
| 1.1 | `e637ae3` | SQL-backed tests on recent Qt |
| 1.2 | `d008dfe` | CMake 3.25 and C++23 |
| 1.3 | `445e281` | Drop Git and LinguistTools as requirements |
| 1.4 | `96eb864` | Move the tests to `tests/` |
| 1.5 | `6478283` | Optional, headless tests |
| 1.6a | `a1785a0` | Shared warnings target |
| 1.6b | `a9ef1d0` | Fix the warnings |
| 1.7 | `2f63718` | Sanitizers |
| 1.8 | `f6ad9a7` | Valgrind |
| 1.9 | `431ecdd` | CMake presets |
| 1.10 | `a1c4ed2` | clang-format and editorconfig |
| 1.11 | `04af0e5` | Reformat everything (mechanical, skip it) |
| 1.12a | `a7f67c0` | clang-tidy setup |
| 1.12b | `7116c5e` | Automatic modernization (mechanical) |
| 1.12c | `0803157` | Internal linkage |
| 1.12d | `8502bfa` | `const` and initialization |
| 1.12e | `d063017` | Lambda captures |
| 1.12f | `e913a2e` | Parameter names |
| 1.12g | `1975c4c` | Remaining clang-tidy findings |
| 1.12h | `3ce5cbb` | `std::array` for the SysEx tables |
| 1.13a | `a692917` | cppcheck target |
| 1.13b | `8ea3fe7` | cppcheck findings |
| 1.14 | `630461f` | Coding conventions (read `CONTRIBUTING.md` instead) |
| 1.15 | `51fc83c` | Pre-commit hook |

**Phase 2, CI**

| Item | Commit | Subject |
|---|---|---|
| 2.1 | `6eef68d` | Test workflow |
| 2.2 | `87f3725` | Quality workflow |
| 2.3 | `7c5e4a2` | Release workflow |
| 2.4 | `11330c8` | Dependabot, and CI once per change |

**Phase 3, fixes in the old code.** The ones most worth reading are marked.

| Item | Commit | Subject |
|---|---|---|
| 3.1 | `6b3d080` | Remove dead code |
| 3.2 | `fcc1d51` | Remove Qt 5 compatibility |
| 3.3 | `28fddb7` | Off-by-one `dataChanged` |
| 3.4 | `ba36d2a` | No downcast of the application object |
| 3.5 | `d27ce5e` | **Make `IOMidi` safe:** resource ownership |
| 3.6 | `2775f9b` | **Context object on every lambda `connect`** |
| 3.7 | `91e31ac` | Roll back on failure |
| 3.8 | `64e1f24` | Check bounds before reading |
| 3.9 | `9917b18` | Report schema errors, create it atomically |
| 3.10 | `5f4409b` | **The `QAbstractItemModel` contract** |
| 3.11 | `dd04921` | Validate in `setData` |
| 3.12 | `d14dbc5` | Validate SysEx messages |
| 3.13 | `5d101db` | **Undo commands store identities, not pointers** |
| 3.14 | `a9c7f9d` | Report a database that can't be opened |
| 3.15 | `41cc82d` | Show import and export errors |
| 3.16 | `230e729` | **Qt idioms and safe teardown** |
| 3.17 | `25df6fe` | Named columns |

## Phase 5

| Item | Commit | Subject |
|---|---|---|
| 5.1 | `bd496d7` | Desktop entry, icon and AppStream metadata, with validation tests |
| 5.2 | `980785d` | README, CLAUDE.md, CONTRIBUTING |
| 5.3 | `8df1620` | Release notes (`doc/RELEASE-NOTES.md`) and the steps before tagging |

5.4, retiring the migration, stays open on purpose: it waits a release or two.

## Keeping track

A commit can only link to commits before it, so each checklist link in
`doc/IMPROVEMENTS.md` is added by a later commit. The original 140 commits are
kept in `modernize-before-squash` and `security-before-squash`. Some commit
messages still cite old hashes, and those are found there.
