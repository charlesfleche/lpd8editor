#ifndef ENUMS_H
#define ENUMS_H

#include <QtCore>

enum MidiDataRole {
    MidiValueType = Qt::UserRole + 1
};

enum MidiType {
    DefaultType,
    NoteType,
    ToggleType
};

#endif
