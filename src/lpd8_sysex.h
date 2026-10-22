#ifndef LPD8_SYSEX_H
#define LPD8_SYSEX_H

#include <QByteArray>

namespace sysex {

enum Type { TypeInvalid, TypeProgram };

int programRequestSize();
int padsCount();
int knobsCount();

void makeSetProgramRequest(QByteArray &sysex, int programId);

Type type(const QByteArray &sysex);
void addProgramHeader(QByteArray &sysex, int programId);
void addFooter(QByteArray &sysex);
QByteArray getProgram(int programId);

char channel(const QByteArray &sysex);
} // namespace sysex

#endif // LPD8_SYSEX_H
