#ifndef LPD8_SYSEX_H
#define LPD8_SYSEX_H

#include "lpd8_types.h"

#include <QByteArray>

namespace sysex {

enum Type {
    TypeInvalid,
    TypeProgram
};

int padsCount();
int knobsCount();

Type type(const QByteArray&);
void addHeader(QByteArray&);
void addFooter(QByteArray&);
QByteArray getProgram(int);
QByteArray setProgram(pProgram);
pProgram toProgram(const QByteArray&);
void addProgramHeader(QByteArray&, int);

char channel(const QByteArray&);
}

#endif // LPD8_SYSEX_H
