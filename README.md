[![test](https://github.com/charlesfleche/lpd8editor/actions/workflows/test.yml/badge.svg)](https://github.com/charlesfleche/lpd8editor/actions/workflows/test.yml) [![release](https://github.com/charlesfleche/lpd8editor/actions/workflows/release.yml/badge.svg)](https://github.com/charlesfleche/lpd8editor/actions/workflows/release.yml)

# lpd8editor

A Linux editor for the [Akai LPD8 pad controller](http://www.akaipro.com/product/lpd8).

![lpd8-editor screenshot](doc/screenshot.png?raw=true "lpd8editor")

It keeps any number of programs (the note, program change and control change
of each pad, whether it toggles, and the control change and range of each
knob), with undo and redo. It gets a program from the LPD8 or sends one to any
of its four slots, and imports and exports programs as `.lpd8` files.

## Supported devices

Only the original LPD8 is supported. There is however an interest for the [LPD8 Mk2](https://www.akaipro.com/lpd8-mk2/) [#49](https://github.com/charlesfleche/lpd8editor/issues/49) and the [LPD8 Wireless](https://www.akaipro.com/downloads-and-support/downloads/?product=LPD8%20Wireless&legacy=true) [#47](https://github.com/charlesfleche/lpd8editor/issues/47). I don't own these devices but i live in Montréal, Québec: if I can borrow one of those or if you can ship them to me, I'd be happy to support them.

lpd8editor talks to the device through the ALSA sequencer. PipeWire bridges
ALSA sequencer clients, and JACK sees them through pipewire-jack or a2jmidid,
so it works with the usual Linux audio setups. Connect to the LPD8 from the
list in the toolbar, or with another tool (aconnect, qjackctl).

## Where the programs are kept

In `~/.local/share/lpd8editor/lpd8editor/programs.json`, a JSON file saved
after each change. lpd8editor 0.0.18 and earlier kept them in an SQLite
database, `db.sqlite3`, in the same folder: the first start of a newer version
moves them to `programs.json` once, keeps the old database as
`db.sqlite3.migrated`, and writes a report of what was moved next to it.

If the programs file can't be read at startup, lpd8editor says why and changes
nothing. A damaged file can be set aside (renamed, never deleted) to start
with no programs; a file written by a newer version of lpd8editor is left for
that version.

## Prebuilt binaries

Check the [releases](https://github.com/charlesfleche/lpd8editor/releases) page for ready to use packages, built for Debian 13 "trixie" and Ubuntu 26.04.

## Building

Building lpd8editor requires a C++23 compiler (GCC 14 or Clang 19 and later), CMake 3.25 or later, Ninja (the presets use it), ALSA and Qt 6.8 or later. On Debian or Ubuntu:

```bash
sudo apt install cmake ninja-build g++ libasound2-dev qt6-base-dev libqt6sql6-sqlite qt6-svg-plugins
```

### Building from git clone

```bash
git clone https://github.com/charlesfleche/lpd8editor.git
cd lpd8editor

# Configure, build and test, into build/gcc-release
cmake --preset gcc-release -DCMAKE_INSTALL_PREFIX=~/lpd8editor
cmake --build --preset gcc-release
ctest --preset gcc-release

# Install, and run
cmake --install build/gcc-release
~/lpd8editor/bin/lpd8editor
```

`cmake --list-presets` lists the other configurations (Clang, debug,
sanitizers, valgrind).

### Building from a compressed source folder

GitHub automatically publishes compressed source folders (.tar.gz or .zip archives) for each [release](https://github.com/charlesfleche/lpd8editor/releases). The version then comes from the folder's name, so keep it as extracted (`lpd8editor-x.y.z`):

```bash
# Replace x.y.z by the version of the downloaded archive
tar xf lpd8editor-x.y.z.tar.gz
cd lpd8editor-x.y.z
cmake --preset gcc-release -DCMAKE_INSTALL_PREFIX=~/lpd8editor
cmake --build --preset gcc-release
cmake --install build/gcc-release
```

### Building the binary packages

```bash
# DEB and TGZ packages, in build/gcc-release/packages
cmake --build --preset gcc-release --target package
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the checks (formatting, clang-tidy,
cppcheck, sanitizers, valgrind) and the coding conventions, and
[doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) for how the code is organised.

## LPD8 sysex

Akai does not provide a MIDI and Sysex implementation sheet, so we [reverse engineered our own](doc/SYSEX.md).
