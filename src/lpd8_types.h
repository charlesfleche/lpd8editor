#ifndef LPD8_TYPES_H
#define LPD8_TYPES_H

#include <QSharedPointer>

struct Pad
{
    char note;
    char pc;
    char cc;
    char toggle;
};

struct Knob
{
    char cc;
    char low;
    char high;
};

struct Program
{
    char id;
    char channel;
    Pad pads[8];
    Knob knobs[8];
};

typedef QSharedPointer<Program> pProgram;

#endif // LPD8_TYPES_H
