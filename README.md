# lpd8-editor

[![Join the chat at https://gitter.im/lpd8-editor/Lobby](https://badges.gitter.im/lpd8-editor/Lobby.svg)](https://gitter.im/lpd8-editor/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

A Linux editor for the [Akai LPD8 pad controller](http://www.akaipro.com/product/lpd8).

![lpd8-editor screenshot](doc/screenshot.png?raw=true "lpd8-editor")

## Dependencies

- alsa
- c++11
- pkg-config
- qt5
  - QtSql
  - QtSVG
  - QtWidgets

## Build and run

``` sh
$ cd lpd8-editor
$ qmake
$ make
$ ./lpd8-editor
```

## Build Debian package

```sh
dpkg-buildpackage -b -rfakeroot -us -uc
```

## LPD8 sysex

Akai does not provide a MIDI and Sysex implementation sheet, so we [reverse engineered our own](doc/SYSEX.md).
