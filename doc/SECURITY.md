# Security

lpd8editor is a desktop application, run by one user, without privileges and
without network code. This document says what it can be attacked through,
what the security review found there, and the rules that keep it fixed.

The review was first done on the code before the architecture rework
(SQLite storage, `IOMidi`, `SysexHandler`, `lpd8_sysex.*`), on the
`security-review` branch, which is kept as the record. The rework replaced
most of that code, so the findings were checked again against the new code
(`AlsaSeqTransport`, `SysexAssembler`, `Lpd8Device`, `lpd8codec`,
`ProgramStore`, `programfile`, the migration, and `App` with its dialogs
`AppDialogs`, and `MainWindow::showProblem()`), and the new code was
reviewed for the same classes of bugs. This document describes the new
code.

## Threat model

| Input | Who controls it | Read by |
|---|---|---|
| ALSA sequencer events | **Any local process** that can open `/dev/snd/seq`: other sessions on the same seat (logind gives the device to the active session), members of the `audio` group, sandboxed applications given the sound device. Sending to our port needs **no subscription**. The kernel sets the sender's client id, and only accepts one of the sender's own ports as the source port; the event type, its flags and its content are the sender's choice. Any client can also create up to 254 ports, and subscribe its ports to ours. | `AlsaSeqTransport` → `SysexAssembler` → `Lpd8Device`; the port list and the connection menu |
| Program files (`.lpd8`) | Whoever wrote the file: presets downloaded or shared. | `importProgramFile()` → `fromSysexTextFile()` → `decodeProgram()` |
| File and folder names | The same, for example the folders of an extracted archive. | Error messages, program names, the export dialog |
| Program names | The user, imported file names, old databases. | The store, the export dialog, messages |
| `db.sqlite3` | Every version of lpd8editor that wrote it. Versions 0.0.0 to 0.0.5 created the tables **without CHECK constraints** (v0.0.6 is the first with them), stored any value they were given, and SQLite never adds constraints to an existing table. | `migrateSqliteDatabase()`, once, from `App::start()`; its report and errors in `AppDialogs` |
| `programs.json` | This application, and the user editing it by hand. | `ProgramStore::load()`, at every start, from `App::start()` |
| The LPD8 | A trusted device, but its replies reach us through the same port as everyone else's events. | As ALSA events |

Out of scope: an attacker who can already write the user's files (they can
edit `~/.bashrc`), the kernel, and the libraries (Qt, alsa-lib, SQLite).
Files of the application's data folder are covered for robustness, not as
an attack.

## Findings

Every finding that still applied was reproduced first: each fix starts with
a test that fails (or hangs, or crashes) without it. The fixes are named by
commit subject, since the hashes change when the branch is rebased.

### The findings of the first review, against the new code

| # | Finding | Status in the new code |
|---|---|---|
| 1 | **Read at an address chosen by another process.** A SysEx event with the `VARUSR` length type carries a pointer into the *sender's* memory, which the kernel delivers unchanged. | **Came back** in `AlsaSeqTransport`, which read `ext.ptr` of every SysEx. Fixed by "Drop MIDI events that point into another process". |
| 2 | **Program overwritten by any local client.** Any program dump, from anyone, at any time, replaced the selected program. | **Fixed by design** in `Lpd8Device` (item 4.10): a dump is taken only as the reply to the request in flight, for the slot asked for, from a port the application receives from. Covered by `lpd8device_test` with a fake transport, and now end to end over ALSA by "Test that a reply forged over ALSA is not taken for the device's". |
| 3 | **Out-of-bounds write when sending** a program that storage failed to read. | **Obsolete.** The code is gone; `encodeSendProgram()` returns `std::expected` and refuses an invalid slot or program. |
| 4 | **Out-of-bounds read when displaying** a stored value outside its lookup table. | **Obsolete.** `ProgramStore` only ever holds valid programs: every way in (`add`, `setProgram`, `fromJson()`, the migration, the models' `setData()` through `exactInteger()`) checks `Program::isValid()`, so a value always indexes its table. |
| 5 | **Window frozen by a file**: importing a FIFO blocked forever, `/dev/zero` never ends. | **Still there** (the import calls the same reader). Fixed by "Parse program files strictly, and only read regular files". |
| 6 | **Local files loaded by an error message**: `QMessageBox` guesses HTML, and a folder named `<img src="` made Qt open the file it names. | **Came back** with the static `QMessageBox` functions, and in the boxes of the new code. Fixed by "Show error messages as plain text": `messagebox.h`, App's dialogs (`MessageBoxDialogs`: the startup questions, the migration and set-aside notices, the confirmation to close without saving at quit and at the end of the session), and the window's problem box (`MainWindow::showProblem()`). |
| 7 | **Window frozen by a flood** of MIDI events. | **Came back** in `AlsaSeqTransport::readEvents()`. Fixed by "Return to the event loop while a MIDI client floods our port". |
| 8 | **Forged port announcements** removed a live port from the list. | **Came back** in `AlsaSeqTransport`. Fixed by "Ignore port announcements that don't come from the system". |
| 9 | **Lenient file parser**: hexadecimal, octal, signs, integers that wrap around. | **Still there.** Fixed with finding 5. |
| 10 | **Out-of-range values on the MIDI path.** | **Fixed by design**: `lpd8codec` checks every data byte both ways, and `MidiTransport::sendSysex()` refuses anything but F0, data bytes, F7. |
| 11 | **Export suggested a path out of Documents** for a program named `../../.bashrc`. | **Came back.** Fixed by "Keep the suggested export file in the Documents folder". |
| 12 | **Start-up hang on a FIFO as the programs file**, and a read not bounded by the size limit. | **Fixed by the rework** (item 4.11a): `ProgramStore::load()` checks the file type with `std::filesystem::status()` before opening it, and reads at most the limit plus one byte. "Check that the programs file is still regular after opening it" adds `isSequential()` after `open()`: a character device swapped in between (a tty, `/dev/input/*`) opens at once and would block in `read()`, which the bound doesn't prevent. |
| 13 | **Saving could lock the user out**: `save()` wrote a file larger than `load()` accepts. | **Still there.** Fixed by "Never save a programs file that can't be loaded again". |

### New findings in the new code

| # | Finding | Attacker | Fixed by |
|---|---|---|---|
| 14 | **Unbounded reassembly state.** `AlsaSeqTransport` kept one `SysexAssembler` per source port ever seen, each holding up to 4 KB of a message that never ends. One client with 254 ports kept about 1 MB for the life of the application, 64 clients about 64 MB. | Local ALSA client | "Keep unfinished SysEx messages from at most 16 sources" |
| 15 | **Start-up hang on a FIFO as the old database, or next to it**: SQLite's `open()` blocks on it, before any window, and SQLite also opens `-journal`, `-wal` and `-shm`. | The data folder (robustness) | "Migrate the old database only if it is a regular file", "Refuse a FIFO next to the old database, where SQLite opens its journal" |

### Accepted risks

- **A client patched into our port can answer a request.** Any ALSA client
  can subscribe its port to ours, as `aconnect` does, and then it is "a
  port the application receives from": it can answer a 'get program' in
  flight, for the slot asked, within the 2 s timeout. Connections made by
  other tools (qjackctl, JACK bridges) must keep working, so this is
  accepted (doc/ARCHITECTURE.md, "Device"). Such a connection is not
  announced: the toolbar's combo box just shows no selection then, and is
  disabled.
- **What we send goes to every subscriber**, so a client subscribed to our
  port sees the programs sent to the device. Programs are not secret.
- **A flood still degrades MIDI.** The fixes keep the window responsive and
  the memory bounded, but a client that floods our port can still make
  requests time out, or evict an unfinished message of the device.
- **Files can be swapped between the check and the open.** A device
  swapped in is caught after opening (`isSequential()`), so it can't block
  a read; a FIFO swapped in blocks in `open()` itself, before any check
  after it can run. SQLite opens the old database and its journal itself,
  so the migration has no check after opening at all. Either way the
  attacker needs write access to the folder.
- **An old database with a gigantic program name** makes the migration
  fail at every start (it would write a file that can't be loaded), with a
  message saying nothing was changed. No such name could be typed in the old
  versions' editor.

## What keeps them fixed

- **Fuzzing** (`fuzz/`, see `fuzz/README.md`): the codec, the SysEx
  assembler, the program file parser, the JSON programs file and the
  migration from old databases are fuzzed under ASan and UBSan for
  properties, not only crashes. CI fuzzes each target on every pull
  request, and every build replays the seeds as tests (`fuzz_<name>`).
- **ALSA tests on CI**: the runner loads the sequencer module, and the tests
  must not skip there (`LPD8EDITOR_REQUIRE_ALSA`), so the tests of findings
  1, 2, 7, 8 and 14, which play a hostile ALSA client, run on every pull
  request.
- **Hardened binaries** (`cmake/Hardening.cmake`): stack protector,
  `_FORTIFY_SOURCE=3` in optimized builds, `_GLIBCXX_ASSERTIONS`, PIE, full
  RELRO, a non-executable stack. The `hardening` test checks all but
  `_GLIBCXX_ASSERTIONS`, which leaves no trace, in the binary.
- **CI**: actions pinned to a commit, least-privilege permissions, no
  credentials left in checkouts, signed build provenance for releases.

## Rules for new code

- **Anything from outside is validated where it enters, at run time.**
  `Q_ASSERT` is for programmer errors only (CONTRIBUTING.md). A bad file, an
  odd event or an old database is not one.
- **MIDI events are from strangers.** Never read `data.ext.ptr` of an event
  that is not `SND_SEQ_EVENT_LENGTH_VARIABLE`. Trust `source.client` (the
  kernel sets it), never the event type or the content: announcements come
  from `SND_SEQ_CLIENT_SYSTEM` only. Take a reply only from a connected
  port, for a request we made (`Lpd8Device`). Keep no state per source that
  isn't bounded, since a client can have 254 ports. Never read events
  without a budget.
- **Text from outside is plain text.** Use `messagebox.h` for any message
  that contains a path, a file name or a program name, set
  `Qt::PlainText` on any box built by hand (as `MessageBoxDialogs` and
  `MainWindow::showProblem()` do), and never set such text as rich text on
  a label. A new `AppDialogs` implementation follows the same rule.
- **A name is not a path.** Make a file name from it with `fileNameFor()`.
- **A new parser gets a fuzzer** in the same commit, checking properties
  (no crash, round trip, what is accepted is valid), as `fuzz/` does.
- **Files are read bounded, and only if regular**: a size limit enforced on
  the read itself, not only on `size()`, and no FIFO or device, which would
  block or never end. Check before opening, since opening is what blocks
  (`fromSysexTextFile()`, `ProgramStore::load()`, the migration).
- **A writer enforces its reader's limits**: never write a file that the
  application would refuse to read (`ProgramStore::save()`).
- **A bug fix starts with a test that fails**, and a fuzzer finding goes into
  the fuzzer's corpus.
