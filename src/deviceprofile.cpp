#include "deviceprofile.h"

namespace lpd8 {

namespace {

Program factoryProgram()
{
    // The program the LPD8 ships with. These are also the values of the SQL
    // trigger that created new programs until now
    // (create_trigger_add_program.sql), one row per line.
    Program program;
    program.channel = 9; // MIDI channel 10, the usual drum channel
    program.pads = {{
            {.note = 36, .programChange = 1, .controlChange = 48, .toggle = false},
            {.note = 37, .programChange = 2, .controlChange = 49, .toggle = false},
            {.note = 38, .programChange = 3, .controlChange = 50, .toggle = false},
            {.note = 39, .programChange = 4, .controlChange = 51, .toggle = false},
            {.note = 40, .programChange = 5, .controlChange = 16, .toggle = false},
            {.note = 41, .programChange = 6, .controlChange = 17, .toggle = false},
            {.note = 42, .programChange = 7, .controlChange = 18, .toggle = false},
            {.note = 43, .programChange = 8, .controlChange = 19, .toggle = false},
    }};
    program.knobs = {{
            {.controlChange = 16, .low = 0, .high = 127},
            {.controlChange = 17, .low = 0, .high = 127},
            {.controlChange = 18, .low = 0, .high = 127},
            {.controlChange = 19, .low = 0, .high = 127},
            {.controlChange = 48, .low = 0, .high = 127},
            {.controlChange = 49, .low = 0, .high = 127},
            {.controlChange = 50, .low = 0, .high = 127},
            {.controlChange = 51, .low = 0, .high = 127},
    }};
    return program;
}

} // namespace

const DeviceProfile &profile()
{
    // A function-local static is built on first use, once, even with threads
    // (C++11 'magic statics'), and avoids the initialization-order problems
    // of a global object.
    static const DeviceProfile lpd8{
            .name = QStringLiteral("LPD8"),
            .programSlots = 4,
            .defaultProgram = factoryProgram(),
    };
    return lpd8;
}

} // namespace lpd8
