#include "lpd8_sysex.h"

static const char sysex_start = '\xf0';
static const char sysex_manufacturer = '\x47';
static const char sysex_model_byte_1 = '\x7f';
static const char sysex_model_byte_2 = '\x75';
static const char sysex_end = '\xf7';

static const int channel_offset = 8;

static const int opcode_offset = 4;
static const int program_offset = 7;

static const int minimum_rep_sysex_size = 9;

static const char sysex_program_req[] = {'\x63', '\x00', '\x01'};
static const char sysex_program_rep[] = {'\x63', '\x00', '\x3a'};
static const char sysex_program_rep_size = 66;
static const char sysex_program_rep_payload_offset = 7;

static const char sysex_set_program_req[] = {'\x61', '\x00', '\x3a'};
static const char sysex_set_program_req_size = 66;


#define RETURN_IF_TYPE(type, b, code, size) if (is(b, code, size)) return type

namespace sysex {

int padsCount() {
    return 8;
}

int knobsCount() {
    return 8;
}

// Caller handle bound exceptions
bool is(const QByteArray& b, const char* code, int size) {
    for (int i = 0 ; i < 3 ; ++i) {
        if (b[opcode_offset + i] != code[i]) {
            return false;
        }
    }
    return b.size() == size;
}

Type type(const QByteArray& b) {
    if (b.size() < minimum_rep_sysex_size
        || b[0] != sysex_start
        || b[b.size()-1] != sysex_end) {
        return TypeInvalid;
    }

    RETURN_IF_TYPE(TypeProgram, b, sysex_program_rep, sysex_program_rep_size);

    return TypeInvalid;
}

void addHeader(QByteArray& b) {
    b += sysex_start;
    b += sysex_manufacturer;
    b += sysex_model_byte_1;
    b += sysex_model_byte_2;
}

void addFooter(QByteArray& b) {
    b += sysex_end;
}

void addProgramHeader(QByteArray& b, int programId) {
    addHeader(b);
    b += sysex_program_req[0];
    b += sysex_program_req[1];
    b += sysex_program_req[2];
    b += '\x00' + programId;
}

QByteArray getProgram(int programId) {
    QByteArray ret;
    addHeader(ret);
    ret += sysex_program_req[0];
    ret += sysex_program_req[1];
    ret += sysex_program_req[2];
    ret += '\x00' + programId;
    addFooter(ret);
    return ret;
}

void makeSetProgramRequest(QByteArray &sysex, int programId) {
    Q_ASSERT(sysex.count() == sysex_set_program_req_size);
    Q_ASSERT(programId >= 1 && programId <= 4);

    sysex[opcode_offset + 0] = sysex_set_program_req[0];
    sysex[opcode_offset + 1] = sysex_set_program_req[1];
    sysex[opcode_offset + 2] = sysex_set_program_req[2];
    sysex[program_offset] = static_cast<char>(programId);
}

char channel(const QByteArray &sysex) {
    return sysex[channel_offset];
}

}
