#include "sysexassembler.h"

namespace lpd8 {

namespace {

constexpr unsigned char sysex_start = 0xf0;
constexpr unsigned char sysex_end = 0xf7;
constexpr unsigned char status_bit = 0x80;
constexpr unsigned char realtime_first = 0xf8;

} // namespace

SysexAssembler::SysexAssembler(qsizetype maxSize)
    : m_maxSize(maxSize)
{
    Q_ASSERT(maxSize >= 2);
}

SysexAssembler::Result SysexAssembler::feed(QByteArrayView bytes)
{
    Result result;
    for (const char c : bytes) {
        feedByte(c, result);
    }
    return result;
}

void SysexAssembler::feedByte(char c, Result &result)
{
    // As unsigned: whether char is signed depends on the platform.
    const auto byte = static_cast<unsigned char>(c);

    if (byte >= realtime_first) {
        return; // real time: never part of a message, never ends one
    }
    if (byte == sysex_start) {
        if (m_state == State::InMessage) {
            ++result.aborted;
        }
        m_buffer = QByteArray(1, c);
        m_state = State::InMessage;
        return;
    }
    if (m_state == State::Idle) {
        return;
    }
    if (byte == sysex_end) {
        if (m_state == State::InMessage) {
            m_buffer += c;
            result.messages.append(m_buffer);
            m_tooLongReported = false;
        }
        m_buffer.clear();
        m_state = State::Idle;
        return;
    }
    if ((byte & status_bit) != 0) {
        // Any other status byte: the message is cut.
        if (m_state == State::InMessage) {
            ++result.aborted;
        }
        m_buffer.clear();
        m_state = State::Idle;
        return;
    }
    if (m_state == State::Skipping) {
        return;
    }
    // A data byte. The final F7 must still fit.
    if (m_buffer.size() + 2 > m_maxSize) {
        m_buffer.clear();
        m_state = State::Skipping;
        if (!m_tooLongReported) {
            m_tooLongReported = true;
            result.tooLongReported = true;
        }
        return;
    }
    m_buffer += c;
}

} // namespace lpd8
