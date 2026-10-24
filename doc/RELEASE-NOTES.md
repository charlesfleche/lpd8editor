# Release notes

What changed for users and packagers, release by release. The source of
these notes is the checklist in `doc/IMPROVEMENTS.md` (item 5.3), where each
commit that changes behaviour adds a line; this file says the same for
readers who don't follow the code.

## Next release (since 0.0.18)

A large rework of the inside of lpd8editor: the storage, the MIDI code and
the window were rewritten, with tests for each part. Most of it is
invisible; this is what you can notice.

### Your programs move to a new file

- The programs are now kept in `programs.json`, in the same folder as
  before (`~/.local/share/lpd8editor/lpd8editor/`), instead of the SQLite
  database `db.sqlite3`. The first start of this version moves them once,
  keeps the old database as `db.sqlite3.migrated`, and shows a report of
  what was moved, also saved next to the old database. Programs that can't
  be moved (a value out of range, which databases from before 2018 can
  hold) are listed in the report, with the reason, and stay in the old
  database.
- The file is saved after each change, and written safely: a crash during
  a save leaves the previous version intact. Quitting without an edit
  writes nothing.
- When something goes wrong at startup, lpd8editor says what and changes
  nothing, and offers what fits:
  - The old database can't be read: Retry, after closing the program
    using it. A database that a crash left half-written gets the
    instructions to repair it.
  - The programs file is damaged: Set aside renames it (it is never
    deleted), and lpd8editor starts with no programs.
  - The file was written by a newer version of lpd8editor, or can't be
    read (permissions): Quit, with what to do.
- When the programs can't be saved (a full disk, a folder you can't write
  to), editing goes on and the problem is shown in a box that doesn't block
  the window. Closing the window then asks whether to close without saving
  or to cancel and keep the window open; before, the changes were lost.

### MIDI and the LPD8

- **Only the answer to your own request is applied** ('Get program N'),
  and only from a port connected to lpd8editor. Before, any program dump
  that arrived, from any MIDI client and whether asked for or not,
  overwrote the selected program. Connections made with aconnect, qjackctl
  or a JACK bridge keep working.
- The answer goes to the program that was selected when you chose 'Get
  program N', even if another one is selected by the time it arrives.
  Before, it went to the selection at its arrival.
- While the LPD8 is asked for a program, the MIDI actions are off. A
  device that doesn't answer within 2 seconds, or that is unplugged
  meanwhile, is reported in one message that doesn't block the window.
- Devices plugged in or out are seen after using the connection menu too.
  Before, connecting or choosing 'Disconnect' stopped the list from
  following them.
- 'Send to program N' and 'Get program N' send nothing when nothing
  receives from lpd8editor's port; before, the message went to no one,
  with the same effect.
- A message from another manufacturer's device that happens to look like a
  program dump is no longer taken for an LPD8 program.

### Editing, import and export

- The pads and knobs tables have readable headers ("Note", "Program
  change", "Control change", "Mode"; "Control change", "Low", "High")
  instead of the database's column names ("note", "pc", "cc", "toggle").
- Fixed a crash: editing a pad, deleting the program, then undoing twice.
- Editing a value without changing it no longer adds an undo step.
- Import and export say what went wrong (the file can't be read, it ends
  too early, value N is invalid) instead of failing silently, and a
  failed export never leaves a truncated file.
- Their file dialogs show LPD8 programs (`*.lpd8`) first, and export
  suggests `<name>.lpd8`. An imported program is named after the file
  without its extension: `Drums.lpd8` becomes "Drums".
- A program file with more than 66 values (two programs back to back, for
  example) is refused. Before, its first program was imported without a
  word.
- A program message one byte short is refused. Before, it was accepted,
  with the last knob value read as 0.
- The note D sharp is written D♯, like the other sharps, instead of D#.
- Importing a file that isn't an LPD8 program is refused with a message.
  That includes a file whose slot byte (the 8th value) is not 1 to 4.
  Files exported by lpd8editor always have slot 1, so only files from
  elsewhere are affected. A failed import no longer leaves a half-made
  program behind.
- The same rule holds for the `default/sysex` setting, the template of new
  programs: a template that isn't an LPD8 program is ignored, with a
  warning, and the factory program is used.

### Desktop integration

- lpd8editor appears in the desktop's application menu, with its icon, and
  in software centers (GNOME Software, KDE Discover).

### Security

The fix of the MIDI program dumps above ("Only the answer to your own
request is applied") came out of a security review.

<!-- Security fixes from the security review branch add their lines here,
     when they are merged. -->

### For packagers and people who build lpd8editor

- Building needs a C++23 compiler (GCC 14 or Clang 19), CMake 3.25 and Qt
  6.8 or later. Git is optional (a source archive takes its version from
  the folder name), and Qt LinguistTools is no longer needed.
- The packages depend on `libqt6sql6-sqlite`, for the one-time migration
  of the old database, and `qt6-svg-plugins`, for the icon.
- The install adds `share/applications/io.github.charlesfleche.lpd8editor.desktop`,
  `share/metainfo/io.github.charlesfleche.lpd8editor.metainfo.xml` and the
  icon in `share/icons/hicolor/scalable/apps/`.
- Release packages are built per distribution, Debian 13 and Ubuntu 26.04
  (`lpd8editor_<version>-1~debian13_amd64.deb`), with a `SHA256SUMS` file.

### Known issues

- The pads and knobs tables are empty until a program is selected, so the
  first selection can make a window at its minimum width about 28 pixels
  wider, once.
- A later release will stop reading `db.sqlite3` (checklist item 5.4).
  Upgrade through this release, or one soon after it, to keep the
  migration.
