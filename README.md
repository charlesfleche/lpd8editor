# lpd8-editor

A Linux editor for the [Akai LPD8 pad controller](http://www.akaipro.com/product/lpd8).

![lpd8-editor screenshot](doc/screenshot.png?raw=true "lpd8-editor")

## Dependencies

- Qt 5.8
- Alsa

## Build and run

``` sh
$ qmake
$ make
$ ./lpd8-editor
```

Then connect the LPD8 with qjackctl:

![qjackctl screenshot](doc/qjackctl.png?raw=true "qjackctl connections")
