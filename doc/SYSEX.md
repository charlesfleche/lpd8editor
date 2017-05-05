# LPD8 Sysex

## Get program

### Request

```
00   : F0
01   : 47
02-03: 7F 75
04-07: 63 00 01 0X
08   : F7
```

### Response

```
00   : F0          Sysex start
01   : 47          Manufacturer Akai
02-03: 7F 75       Model LPD8
04-07: 63 00 3A 0X Receive Program X=[1-4]
08   : 06          Midi channel ?
09-12: WW XX YY 0Z Pad 1 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
13-16: WW XX YY 0Z Pad 2 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
17-20: WW XX YY 0Z Pad 3 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
21-24: WW XX YY 0Z Pad 4 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
25-28: WW XX YY 0Z Pad 5 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
29-32: WW XX YY 0Z Pad 6 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
33-36: WW XX YY 0Z Pad 7 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
37-40: WW XX YY 0Z Pad 8 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
41-43: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
44-46: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
47-49: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
50-52: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
53-55: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
56-58: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
59-61: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
62-64: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
65   : F7          Sysex end
```

### Example

```
Request:  F0 47 7F 75 63 00 01 01 F7
Response: F0 47 7F 75 63 00 3A 01 06 0C 0D 0E 00 0F 10 11 00 12 13 14 01 15 16 17 00 00 01 02 00 03 04 05 00 06 07 08 00 09 0A 0B 00 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F F7
```

## Send program

### Request

```
00   : F0          Sysex start
01   : 47          Manufacturer Akai
02-03: 7F 75       Model LPD8
04-07: 61 00 3A 0X Send Program X=[1-4]
08   : 0C          Midi channel C=[0-F]
09-12: WW XX YY 0Z Pad 1 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
13-16: WW XX YY 0Z Pad 2 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
17-20: WW XX YY 0Z Pad 3 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
21-24: WW XX YY 0Z Pad 4 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
25-28: WW XX YY 0Z Pad 5 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
29-32: WW XX YY 0Z Pad 6 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
33-36: WW XX YY 0Z Pad 7 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
37-40: WW XX YY 0Z Pad 8 WW=Note XX=Program Change YY=Control Change Z=0,Momentary Z=1,Toggle
41-43: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
44-46: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
47-49: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
50-52: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
53-55: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
56-58: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
59-61: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
62-64: YY LL HH    Knob 1 YY=Control Change LL=Low HH=Hi
65   : F7          Sysex end
```

### Receive

```
None
```

### Example

```
Request:  F0 47 7F 75  61 00 3A 04  06  0C 0D 0E 00  0F 10 11 00  12 13 14 01  15 16 17 00  00 01 02 00  03 04 05 00  06 07 08 00  09 0A 0B 00  18 19 1A  1B 1C 1D  1E 1F 20  21 22 23  24 25 26  27 28 29  2A 2B 2C  2D 2E 2F  F7
Response: None
```

## Get active program

### Request

```
00   : F0       Sysex start
01   : 47       Manufacturer Akai
02-03: 7F 75    Model LPD8
04-07: 64 00 00 Request active program
08   : F7       Sysex ends
```

### Response

```
F0 47 7F 75 64 00 01 01 F7

00   : F0       Sysex start
01   : 47       Manufacturer Akai
02-03: 7F 75    Model LPD8
04-07: 64 00 01 Request active program
08   : 0X       Active program X=[1-4]
09   : F7       Sysex ends
```

### Example

```
Request:  F0 47 7F 75 64 00 00 F7
Response: F0 47 7F 75 64 00 01 01 F7
```

## Activate program

### Request

```
00   : F0          Sysex start
01   : 47          Manufacturer Akai
02-03: 7F 75       Model LPD8
04-07: 62 00 01 0X Activate program X=[1-4]
08   : F7          Sysex ends
```

### Receive

```
None
```

### Example

```
Request:  F0 47 7F 75 62 00 01 01 F7 
Response: None
```

## Query device info

### Request

```
00   : F0    Sysex start
01   : 7E    Non real time SySex
02   : XY    Device ID X=[0-7] Y=[0-F]
03-04: 06 01 Identity request
05   : F7    Sysex ends
```

### Response

```
00   : F0          Sysex start
01   : 7E          Non real time SySex
02   : 00          Device ID
03-04: 06 02       Identity reply
05   : 47          Manufacturer Akai
06-07: 75 00       Familly idea
08-09: 19 00       Product idea
10-13: XX XX YY XX Version, only YY seems to be read by the official LPD8 editor
14-33: 00 * 20     20 bytes of padding
34   : F7          Sysex end
```

### Example

```
Request:  F0 7E 00 06 01 F7
Response: F0 7E 00 06 02 47 75 00 19 00 00 00 66 7F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 F7
```
