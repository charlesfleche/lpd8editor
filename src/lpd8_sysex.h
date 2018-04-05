#ifndef LPD8_SYSEX_H
#define LPD8_SYSEX_H

#include <QByteArray>

namespace sysex {

enum Type {
    TypeInvalid,
    TypeProgram
};

int programRequestSize();
int padsCount();
int knobsCount();

void makeSetProgramRequest(QByteArray &sysex, int programId);

Type type(const QByteArray&);
void addProgramHeader(QByteArray&, int);
void addFooter(QByteArray&);
QByteArray getProgram(int);

char channel(const QByteArray&);
}

#endif // LPD8_SYSEX_H
