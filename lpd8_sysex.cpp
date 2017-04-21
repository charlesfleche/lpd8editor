#include "lpd8_sysex.h"

static const char sysex_start = '\xf0';
static const char sysex_manufacturer = '\x47';
static const char sysex_model_byte_1 = '\x7f';
static const char sysex_model_byte_2 = '\x75';
static const char sysex_end = '\xf7';

static const int opcode_offset = 4;
static const int minimum_rep_sysex_size = 9;

static const char sysex_program_req[] = {'\x63', '\x00', '\x01'};
static const char sysex_program_rep[] = {'\x63', '\x00', '\x3a'};
static const char sysex_program_rep_size = 66;
static const char sysex_program_rep_payload_offset = 7;

static const char sysex_set_program_req[] = {'\x61', '\x00', '\x3a'};

#define RETURN_IF_TYPE(type, b, code, size) if (is(b, code, size)) return type

namespace sysex {

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

pProgram toProgram(const QByteArray & b) {
    Q_ASSERT(type(b) == TypeProgram);

    pProgram ret(new Program);
    int offset = sysex_program_rep_payload_offset;
    ret->id = b[offset++];
    ret->channel = b[offset++];
    for (int i = 0 ; i < 8 ; ++i) {
        ret->pads[i].note = b[offset++];
        ret->pads[i].pc = b[offset++];
        ret->pads[i].cc = b[offset++];
        ret->pads[i].momentary = b[offset++];
    }
    for (int i = 0 ; i < 8 ; ++i) {
        ret->knobs[i].cc = b[offset++];
        ret->knobs[i].low = b[offset++];
        ret->knobs[i].high = b[offset++];
    }
    return ret;
}

QByteArray setProgram(pProgram p) {
    Q_CHECK_PTR(p);

    QByteArray ret;
    addHeader(ret);
    ret += sysex_set_program_req[0];
    ret += sysex_set_program_req[1];
    ret += sysex_set_program_req[2];
    ret += p->id;
    ret += p->channel;
    for (int i = 0 ; i < 8 ; ++i) {
        const Pad& pad = p->pads[i];
        ret += pad.note;
        ret += pad.pc;
        ret += pad.cc;
        ret += pad.momentary;
    }
    for (int i = 0 ; i < 8 ; ++i) {
        const Knob& knob = p->knobs[i];
        ret += knob.cc;
        ret += knob.low;
        ret += knob.high;
    }
    addFooter(ret);
    return ret;
}

}
