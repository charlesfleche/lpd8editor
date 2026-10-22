#ifndef DEVICEPROFILE_H
#define DEVICEPROFILE_H

#include "program.h"

#include <QString>

namespace lpd8 {

// The facts about the device that are not part of a Program or of the SysEx
// encoding: its name, how many programs it stores, and its factory program.
// The number of pads and knobs is fixed by the Program type (8 of each).
// A device with another program layout, such as the LPD8 Mk2, would need
// its own Program type and codec, not only another profile.
struct DeviceProfile
{
    QString name;
    int programSlots{0}; // numbered 1 to programSlots
    Program defaultProgram;
};

const DeviceProfile &profile();

} // namespace lpd8

#endif // DEVICEPROFILE_H
