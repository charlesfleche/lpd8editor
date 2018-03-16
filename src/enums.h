#ifndef ENUMS_H
#define ENUMS_H

#include <QtCore>

enum MidiDataRole {
    MidiValueType = Qt::UserRole + 1,
    MidiValueMin,
    MidiValueMax
};

enum MidiType {
    DefaultType,
    NoteType,
    ToggleType,
    StringType,
    ChannelType
};

#endif
