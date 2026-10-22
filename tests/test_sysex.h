#ifndef TEST_SYSEX_H
#define TEST_SYSEX_H

#include <QByteArray>

#include <array>

// SysEx messages used as test data. The layout is described in doc/SYSEX.md.
// The 66-byte tables are in this application's storage format: a program
// message with the 'get program' opcode (63 00 01), see "Storage format" in
// doc/SYSEX.md.
// Each table is laid out by hand, one field of the message per row, so
// clang-format is turned off around it.

inline constexpr std::size_t program_sysex_size = 66;
using ProgramSysexBytes = std::array<char, program_sysex_size>;

template<std::size_t N>
QByteArray toByteArray(const std::array<char, N> &bytes)
{
    return {bytes.data(), static_cast<qsizetype>(bytes.size())};
}

// The program the LPD8 ships with.
// clang-format off
inline constexpr ProgramSysexBytes default_sysex_bytes{
        '\xf0',                 // SysEx start
        0x47,                   // manufacturer: Akai
        0x7f, 0x75,             // model: LPD8
        0x63, 0x00, 0x01, 0x01, // opcode: get program (storage format), slot 1
        0x09,                   // MIDI channel 10
        0x24, 0x01, 0x30, 0x00, // pad 1: note, PC, CC, toggle
        0x25, 0x02, 0x31, 0x00, // pad 2
        0x26, 0x03, 0x32, 0x00, // pad 3
        0x27, 0x04, 0x33, 0x00, // pad 4
        0x28, 0x05, 0x10, 0x00, // pad 5
        0x29, 0x06, 0x11, 0x00, // pad 6
        0x2a, 0x07, 0x12, 0x00, // pad 7
        0x2b, 0x08, 0x13, 0x00, // pad 8
        0x10, 0x00, 0x7f,       // knob 1: CC, low, high
        0x11, 0x00, 0x7f,       // knob 2
        0x12, 0x00, 0x7f,       // knob 3
        0x13, 0x00, 0x7f,       // knob 4
        0x30, 0x00, 0x7f,       // knob 5
        0x31, 0x00, 0x7f,       // knob 6
        0x32, 0x00, 0x7f,       // knob 7
        0x33, 0x00, 0x7f,       // knob 8
        '\xf7',                 // SysEx end
};
// clang-format on
inline const QByteArray default_sysex = toByteArray(default_sysex_bytes);

// A valid program where every value is 0.
// clang-format off
inline constexpr ProgramSysexBytes allzero_sysex_bytes{
        '\xf0',                 // SysEx start
        0x47,                   // manufacturer: Akai
        0x7f, 0x75,             // model: LPD8
        0x63, 0x00, 0x01, 0x01, // opcode: get program (storage format), slot 1
        0x00,                   // MIDI channel 1
        0x00, 0x00, 0x00, 0x00, // pad 1: note, PC, CC, toggle
        0x00, 0x00, 0x00, 0x00, // pad 2
        0x00, 0x00, 0x00, 0x00, // pad 3
        0x00, 0x00, 0x00, 0x00, // pad 4
        0x00, 0x00, 0x00, 0x00, // pad 5
        0x00, 0x00, 0x00, 0x00, // pad 6
        0x00, 0x00, 0x00, 0x00, // pad 7
        0x00, 0x00, 0x00, 0x00, // pad 8
        0x00, 0x00, 0x00,       // knob 1: CC, low, high
        0x00, 0x00, 0x00,       // knob 2
        0x00, 0x00, 0x00,       // knob 3
        0x00, 0x00, 0x00,       // knob 4
        0x00, 0x00, 0x00,       // knob 5
        0x00, 0x00, 0x00,       // knob 6
        0x00, 0x00, 0x00,       // knob 7
        0x00, 0x00, 0x00,       // knob 8
        '\xf7',                 // SysEx end
};
// clang-format on
inline const QByteArray allzero_sysex = toByteArray(allzero_sysex_bytes);

// Like allzero_sysex, but the MIDI channel is out of range (0x16 > 15).
// clang-format off
inline constexpr ProgramSysexBytes invalid_sysex_channel_bytes{
        '\xf0',                 // SysEx start
        0x47,                   // manufacturer: Akai
        0x7f, 0x75,             // model: LPD8
        0x63, 0x00, 0x01, 0x01, // opcode: get program (storage format), slot 1
        0x16,                   // bad MIDI channel
        0x00, 0x00, 0x00, 0x00, // pad 1: note, PC, CC, toggle
        0x00, 0x00, 0x00, 0x00, // pad 2
        0x00, 0x00, 0x00, 0x00, // pad 3
        0x00, 0x00, 0x00, 0x00, // pad 4
        0x00, 0x00, 0x00, 0x00, // pad 5
        0x00, 0x00, 0x00, 0x00, // pad 6
        0x00, 0x00, 0x00, 0x00, // pad 7
        0x00, 0x00, 0x00, 0x00, // pad 8
        0x00, 0x00, 0x00,       // knob 1: CC, low, high
        0x00, 0x00, 0x00,       // knob 2
        0x00, 0x00, 0x00,       // knob 3
        0x00, 0x00, 0x00,       // knob 4
        0x00, 0x00, 0x00,       // knob 5
        0x00, 0x00, 0x00,       // knob 6
        0x00, 0x00, 0x00,       // knob 7
        0x00, 0x00, 0x00,       // knob 8
        '\xf7',                 // SysEx end
};
// clang-format on
inline const QByteArray invalid_sysex_channel = toByteArray(invalid_sysex_channel_bytes);

// A message too short to be a program.
inline constexpr std::array<char, 6> invalid_sysex_short_bytes{
        '\xf0', 0x47, 0x7f, 0x33, 0x00, 0x7f};
inline const QByteArray invalid_sysex_short = toByteArray(invalid_sysex_short_bytes);

#endif
