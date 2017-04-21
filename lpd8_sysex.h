#ifndef LPD8_SYSEX_H
#define LPD8_SYSEX_H

#include "lpd8_types.h"

#include <QByteArray>

namespace sysex {

enum Type {
    TypeInvalid,
    TypeProgram
};

Type type(const QByteArray&);
QByteArray getProgram(int);
QByteArray setProgram(pProgram);
pProgram toProgram(const QByteArray&);

}

#endif // LPD8_SYSEX_H
