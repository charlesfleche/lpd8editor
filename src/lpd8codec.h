#ifndef LPD8CODEC_H
#define LPD8CODEC_H

#include "program.h"

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

#include <expected>

// Encoding and decoding of the LPD8's SysEx messages (doc/SYSEX.md). Pure
// functions: no I/O, no state. This is the reference implementation of the
// format: the database layer stores and reads programs through it. The
// older helpers in lpd8_sysex.cpp still serve the MIDI path until item 4.10
// moves it here too.
namespace lpd8 {

inline constexpr qsizetype program_message_size = 66;

struct DecodeError
{
    enum class Kind {
        NotAProgram, // not an LPD8 program message (size, header, opcode, slot)
        ValueOutOfRange, // a program message with a value the LPD8 doesn't accept
    };

    Kind kind{Kind::NotAProgram};
    // The position of the byte at fault in the message, 0-based like an
    // index, so the user can be told which value is wrong (describe() shows
    // it counting from 1). -1 when no single byte is (wrong size).
    qsizetype offset{-1};

    bool operator==(const DecodeError &) const = default;
};

// A message for the user, with the position of the byte at fault if any.
QString describe(const DecodeError &error);

struct DecodedProgram
{
    Program program;
    int slot{0}; // the device slot named in the message, 1 to profile().programSlots
};

// Decodes a device dump, a 'send program' request, or the format this
// application stores and exports (doc/SYSEX.md, "Storage format").
std::expected<DecodedProgram, DecodeError> decodeProgram(QByteArrayView sysex);

enum class EncodeError {
    InvalidSlot, // not between 1 and profile().programSlots
    InvalidProgram, // !program.isValid(): a value would not fit in a data byte
};

// These two produce messages for the device, so they refuse what would make
// malformed MIDI (a data byte of 0x80 or more) instead of sending it.
//
// 'Get program': asks the device for the program in 'slot'.
std::expected<QByteArray, EncodeError> encodeGetProgram(int slot);
// 'Send program': writes 'program' into the device's 'slot'.
std::expected<QByteArray, EncodeError> encodeSendProgram(const Program &program, int slot);

// The format this application has always used to store and export
// programs: a program message with the 'get program' opcode, for slot 1.
// Kept as is, so that files exported by earlier versions stay readable.
// Precondition: program.isValid() (storage only holds valid programs).
QByteArray encodeStoredProgram(const Program &program);

} // namespace lpd8

#endif // LPD8CODEC_H
