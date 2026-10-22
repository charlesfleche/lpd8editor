#ifndef PROGRAM_H
#define PROGRAM_H

#include <QtGlobal>

#include <array>
#include <cstddef>

// The contents of one LPD8 program, as plain values. These types have no
// identity and no behaviour: they are copied, compared with ==, and passed
// around. The SysEx encoding is in lpd8codec.h, the storage in
// programstore.h.
//
// A Program is what the device stores, and nothing else. The device has no
// names, so the name the user gives a program is not here: it belongs to
// the store, next to the program's id. Two programs are then equal exactly
// when the device would behave the same with either.

namespace lpd8 {

// Every MIDI data value (note, program change, control change, knob range)
// fits in 7 bits.
inline constexpr int midi_value_max = 127;
inline constexpr int midi_channel_max = 15; // channels 1-16 are stored as 0-15

struct Pad
{
    quint8 note{0};
    quint8 programChange{0};
    quint8 controlChange{0};
    bool toggle{false}; // false: momentary, true: toggle

    bool operator==(const Pad &) const = default;
};

struct Knob
{
    quint8 controlChange{0};
    quint8 low{0};
    quint8 high{midi_value_max};

    bool operator==(const Knob &) const = default;
};

inline constexpr std::size_t pad_count = 8;
inline constexpr std::size_t knob_count = 8;

struct Program
{
    quint8 channel{0};
    std::array<Pad, pad_count> pads{};
    std::array<Knob, knob_count> knobs{};

    // Compares every field, in declaration order. Defaulted by the compiler
    // (C++20), so a field added later can't be forgotten.
    bool operator==(const Program &) const = default;

    // True if every value is in the range the LPD8 accepts.
    bool isValid() const;
};

} // namespace lpd8

#endif // PROGRAM_H
