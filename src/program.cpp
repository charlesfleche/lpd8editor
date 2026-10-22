#include "program.h"

#include <algorithm>

namespace lpd8 {

namespace {

bool isMidiValue(quint8 value)
{
    return value <= midi_value_max;
}

} // namespace

bool Program::isValid() const
{
    const auto validPad = [](const Pad &pad) {
        return isMidiValue(pad.note) && isMidiValue(pad.programChange)
                && isMidiValue(pad.controlChange);
    };
    const auto validKnob = [](const Knob &knob) {
        return isMidiValue(knob.controlChange) && isMidiValue(knob.low) && isMidiValue(knob.high);
    };
    return channel <= midi_channel_max && std::ranges::all_of(pads, validPad)
            && std::ranges::all_of(knobs, validKnob);
}

} // namespace lpd8
