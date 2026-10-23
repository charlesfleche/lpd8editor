#ifndef LPD8_SYSEX_H
#define LPD8_SYSEX_H

#include <QByteArray>

// The MIDI messages that SysexHandler sends and recognizes. The storage
// helpers are gone with the SQLite storage (item 4.7b); these move to the
// codec with Lpd8Device (item 4.10).
namespace sysex {

enum Type { TypeInvalid, TypeProgram };

int programRequestSize();

void makeSetProgramRequest(QByteArray &sysex, int programId);

Type type(const QByteArray &sysex);

QByteArray getProgram(int programId);
} // namespace sysex

#endif // LPD8_SYSEX_H
