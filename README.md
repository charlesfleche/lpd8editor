# lpd8editor [![Build Status](https://travis-ci.org/charlesfleche/lpd8editor.svg?branch=master)](https://travis-ci.org/charlesfleche/lpd8editor) [![Join the chat at https://gitter.im/lpd8-editor/Lobby](https://badges.gitter.im/lpd8-editor/Lobby.svg)](https://gitter.im/lpd8-editor/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) 

A Linux editor for the [Akai LPD8 pad controller](http://www.akaipro.com/product/lpd8).

![lpd8-editor screenshot](doc/screenshot.png?raw=true "lpd8editor")

## Prebuilt binaries

Check the [releases](https://github.com/charlesfleche/lpd8editor/releases) page for ready to use packages.

## Building

Building lpd8editor requires a C++-11 compiler, cmake, Alsa and Qt5 with its internationalization tools. The [Github actions](.github/workflows/release.yml) provides detailed step by step to install dependencies, build, test and release.

After dependencies are installed:

```bash
# Clone the code repository
git clone https://github.com/charlesfleche/lpd8editor.git

# Create the build directory
mkdir build-lpd8editor
cd build-lpd8editor

# Configure the build system
cmake ../lpd8editor -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/lpd8editor

# Build and install
cmake --build . --target install

# Run lpd8editor
~/lpd8editor/bin/lpd8editor
```

### Running the tests

```bash
cmake --build . --target test
```

### Building the binary packages

```bash
# Packages will be generated in the `packages` folder
cmake --build . --target package
```

## Want to contribute a translation ?

Although not currently thoroughly documented, adding a new translation is actually easy and should not take more than a few minutes for a full translation. Please [create a GitHub issue](https://github.com/octo-org/octo-repo/issues/new?title=New+translation+request) and we'll guide you.

## LPD8 sysex

Akai does not provide a MIDI and Sysex implementation sheet, so we [reverse engineered our own](doc/SYSEX.md).
