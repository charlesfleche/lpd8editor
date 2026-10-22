#include "lpd8_sysex.h"

#include <QByteArrayView>

#include <algorithm>
#include <array>

namespace {

constexpr char sysex_start = '\xf0';
constexpr char sysex_manufacturer = '\x47';
constexpr char sysex_model_byte_1 = '\x7f';
constexpr char sysex_model_byte_2 = '\x75';
constexpr char sysex_end = '\xf7';

constexpr int channel_offset = 8;

constexpr int opcode_offset = 4;
constexpr int program_offset = 7;

constexpr int minimum_rep_sysex_size = 9;

// The three opcode bytes that follow the header.
using Opcode = std::array<char, 3>;

constexpr Opcode sysex_program_req{'\x63', '\x00', '\x01'};
constexpr Opcode sysex_program_rep{'\x63', '\x00', '\x3a'};
constexpr int sysex_program_rep_size = 66;

constexpr Opcode sysex_set_program_req{'\x61', '\x00', '\x3a'};
constexpr int sysex_set_program_req_size = 66;

} // namespace

#define RETURN_IF_TYPE(type, b, code, size)                                                        \
    if (is(b, code, size))                                                                         \
    return type

namespace sysex {

int programRequestSize()
{
    return sysex_set_program_req_size;
}

int padsCount()
{
    return 8;
}

int knobsCount()
{
    return 8;
}

namespace {

// The caller makes sure that sysex is long enough to hold an opcode.
bool is(const QByteArray &sysex, const Opcode &code, int size)
{
    const QByteArrayView opcode = QByteArrayView(sysex).sliced(opcode_offset, std::ssize(code));
    return std::ranges::equal(opcode, code) && sysex.size() == size;
}

} // namespace

namespace {

// A SysEx message from an LPD8: start byte, Akai, LPD8 model, end byte.
bool isLpd8SysEx(const QByteArray &sysex)
{
    return sysex.size() >= minimum_rep_sysex_size && sysex[0] == sysex_start
            && sysex[1] == sysex_manufacturer && sysex[2] == sysex_model_byte_1
            && sysex[3] == sysex_model_byte_2 && sysex.back() == sysex_end;
}

} // namespace

bool isProgram(const QByteArray &sysex)
{
    return isLpd8SysEx(sysex)
            && (is(sysex, sysex_program_rep, sysex_program_rep_size)
                || is(sysex, sysex_set_program_req, sysex_set_program_req_size)
                // How this application stores programs (see programSysex())
                || is(sysex, sysex_program_req, sysex_program_rep_size));
}

Type type(const QByteArray &sysex)
{
    if (!isLpd8SysEx(sysex)) {
        return TypeInvalid;
    }

    RETURN_IF_TYPE(TypeProgram, sysex, sysex_program_rep, sysex_program_rep_size);

    return TypeInvalid;
}

namespace {

void addHeader(QByteArray &sysex)
{
    sysex += sysex_start;
    sysex += sysex_manufacturer;
    sysex += sysex_model_byte_1;
    sysex += sysex_model_byte_2;
}

} // namespace

void addFooter(QByteArray &sysex)
{
    sysex += sysex_end;
}

void addProgramHeader(QByteArray &sysex, int programId)
{
    addHeader(sysex);
    sysex += sysex_program_req[0];
    sysex += sysex_program_req[1];
    sysex += sysex_program_req[2];
    sysex += static_cast<char>(programId);
}

QByteArray getProgram(int programId)
{
    QByteArray ret;
    addHeader(ret);
    ret += sysex_program_req[0];
    ret += sysex_program_req[1];
    ret += sysex_program_req[2];
    ret += static_cast<char>(programId);
    addFooter(ret);
    return ret;
}

void makeSetProgramRequest(QByteArray &sysex, int programId)
{
    Q_ASSERT(sysex.size() == sysex_set_program_req_size);
    Q_ASSERT(programId >= 1 && programId <= 4);

    sysex[opcode_offset + 0] = sysex_set_program_req[0];
    sysex[opcode_offset + 1] = sysex_set_program_req[1];
    sysex[opcode_offset + 2] = sysex_set_program_req[2];
    sysex[program_offset] = static_cast<char>(programId);
}

char channel(const QByteArray &sysex)
{
    return sysex[channel_offset];
}

} // namespace sysex
