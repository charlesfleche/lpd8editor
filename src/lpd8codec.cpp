#include "lpd8codec.h"

#include "deviceprofile.h"

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <optional>

namespace lpd8 {

namespace {

// Message layout (doc/SYSEX.md).
constexpr quint8 sysex_start = 0xf0;
constexpr quint8 sysex_end = 0xf7;
constexpr quint8 data_byte_max = 0x7f; // MIDI data bytes have bit 7 clear
constexpr std::array<quint8, 3> lpd8_header{0x47, 0x7f, 0x75}; // Akai, LPD8

using Opcode = std::array<quint8, 3>;
constexpr Opcode get_program{0x63, 0x00, 0x01};
constexpr Opcode program_dump{0x63, 0x00, 0x3a};
constexpr Opcode send_program{0x61, 0x00, 0x3a};

constexpr qsizetype header_offset = 1;
constexpr qsizetype opcode_offset = 4;
constexpr qsizetype slot_offset = 7;
constexpr qsizetype channel_offset = 8;
constexpr qsizetype pads_offset = 9;
constexpr qsizetype pad_size = 4; // note, program change, control change, toggle
constexpr qsizetype knobs_offset = pads_offset + (pad_size * static_cast<qsizetype>(pad_count));
constexpr qsizetype knob_size = 3; // control change, low, high
static_assert(knobs_offset + (knob_size * static_cast<qsizetype>(knob_count)) + 1
              == program_message_size);

// The byte at 'offset' as an unsigned value. Whether char is signed depends
// on the platform (signed on x86, unsigned on Arm Linux), so bytes are never
// compared or converted as plain chars.
quint8 byteAt(QByteArrayView sysex, qsizetype offset)
{
    return static_cast<quint8>(sysex[offset]);
}

bool matches(QByteArrayView sysex, qsizetype offset, const auto &bytes)
{
    // The projection converts each char of the message to quint8 before the
    // comparison (see byteAt()).
    const auto asByte = [](char c) { return static_cast<quint8>(c); };
    return std::ranges::equal(sysex.sliced(offset, std::ssize(bytes)), bytes, {}, asByte);
}

bool isValidSlot(int slot)
{
    return slot >= 1 && slot <= profile().programSlots;
}

void append(QByteArray &message, quint8 byte)
{
    message += static_cast<char>(byte);
}

// The common start of every message: SysEx start, Akai, LPD8, opcode, slot.
QByteArray header(const Opcode &opcode, int slot)
{
    QByteArray message;
    append(message, sysex_start);
    for (const quint8 byte : lpd8_header) {
        append(message, byte);
    }
    for (const quint8 byte : opcode) {
        append(message, byte);
    }
    append(message, static_cast<quint8>(slot));
    return message;
}

QByteArray programMessage(const Opcode &opcode, int slot, const Program &program)
{
    Q_ASSERT(isValidSlot(slot) && program.isValid());

    QByteArray message = header(opcode, slot);
    append(message, program.channel);
    for (const Pad &pad : program.pads) {
        append(message, pad.note);
        append(message, pad.programChange);
        append(message, pad.controlChange);
        append(message, pad.toggle ? 1 : 0);
    }
    for (const Knob &knob : program.knobs) {
        append(message, knob.controlChange);
        append(message, knob.low);
        append(message, knob.high);
    }
    append(message, sysex_end);
    Q_ASSERT(message.size() == program_message_size);
    return message;
}

} // namespace

QString describe(const DecodeError &error)
{
    const auto tr = [](const char *text) { return QCoreApplication::translate("lpd8", text); };
    switch (error.kind) {
    case DecodeError::Kind::NotAProgram:
        return tr("This is not an LPD8 program.");
    case DecodeError::Kind::ValueOutOfRange:
        if (error.offset < 0) {
            return tr("This LPD8 program has a value out of range.");
        }
        // Positions shown to the user count from 1, as a person counts the
        // values of a file. error.offset itself is 0-based, like an index.
        return tr("This LPD8 program has a value out of range at position %1.")
                .arg(error.offset + 1);
    }
    return {};
}

std::expected<DecodedProgram, DecodeError> decodeProgram(QByteArrayView sysex)
{
    using Kind = DecodeError::Kind;
    const auto notAProgram = [](qsizetype offset) {
        return std::unexpected(DecodeError{.kind = Kind::NotAProgram, .offset = offset});
    };

    // The frame: size, start and end bytes, header, opcode and slot.
    if (sysex.size() != program_message_size) {
        return notAProgram(-1);
    }
    if (byteAt(sysex, 0) != sysex_start) {
        return notAProgram(0);
    }
    if (byteAt(sysex, program_message_size - 1) != sysex_end) {
        return notAProgram(program_message_size - 1);
    }
    if (!matches(sysex, header_offset, lpd8_header)) {
        return notAProgram(header_offset);
    }
    if (!matches(sysex, opcode_offset, program_dump) && !matches(sysex, opcode_offset, send_program)
        && !matches(sysex, opcode_offset, get_program)) {
        return notAProgram(opcode_offset);
    }
    DecodedProgram decoded;
    decoded.slot = byteAt(sysex, slot_offset);
    if (!isValidSlot(decoded.slot)) {
        return notAProgram(slot_offset);
    }

    // The values. Reading only checks that each one is a MIDI data byte (and
    // that a toggle is 0 or 1, the only byte here with its own encoding).
    // Whether the values make a valid program is Program::isValid()'s
    // decision, so the rules live in one place.
    std::optional<qsizetype> badByte;
    const auto data = [&](qsizetype offset) -> quint8 {
        const quint8 value = byteAt(sysex, offset);
        if (value > data_byte_max && !badByte) {
            badByte = offset;
        }
        return value;
    };

    Program &program = decoded.program;
    program.channel = data(channel_offset);
    qsizetype offset = pads_offset;
    for (Pad &pad : program.pads) {
        pad.note = data(offset);
        pad.programChange = data(offset + 1);
        pad.controlChange = data(offset + 2);
        const quint8 toggle = data(offset + 3);
        if (toggle > 1 && !badByte) {
            badByte = offset + 3;
        }
        pad.toggle = toggle == 1;
        offset += pad_size;
    }
    Q_ASSERT(offset == knobs_offset);
    for (Knob &knob : program.knobs) {
        knob.controlChange = data(offset);
        knob.low = data(offset + 1);
        knob.high = data(offset + 2);
        offset += knob_size;
    }

    if (badByte) {
        return std::unexpected(DecodeError{.kind = Kind::ValueOutOfRange, .offset = *badByte});
    }
    if (!program.isValid()) {
        // Every byte is a data byte, but a field is out of its own range.
        // Today only the channel can be (0-15). If Program::isValid() gains a
        // rule, its offset is unknown here, hence -1.
        const qsizetype at = program.channel > midi_channel_max ? channel_offset : -1;
        return std::unexpected(DecodeError{.kind = Kind::ValueOutOfRange, .offset = at});
    }
    return decoded;
}

std::expected<QByteArray, EncodeError> encodeGetProgram(int slot)
{
    if (!isValidSlot(slot)) {
        return std::unexpected(EncodeError::InvalidSlot);
    }
    QByteArray message = header(get_program, slot);
    append(message, sysex_end);
    return message;
}

std::expected<QByteArray, EncodeError> encodeSendProgram(const Program &program, int slot)
{
    if (!isValidSlot(slot)) {
        return std::unexpected(EncodeError::InvalidSlot);
    }
    if (!program.isValid()) {
        return std::unexpected(EncodeError::InvalidProgram);
    }
    return programMessage(send_program, slot, program);
}

QByteArray encodeStoredProgram(const Program &program)
{
    return programMessage(get_program, 1, program);
}

} // namespace lpd8
