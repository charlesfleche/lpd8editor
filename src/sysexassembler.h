#ifndef SYSEXASSEMBLER_H
#define SYSEXASSEMBLER_H

#include <QByteArray>
#include <QByteArrayView>
#include <QList>

namespace lpd8 {

// Turns the bytes received from one MIDI source into complete SysEx
// messages (doc/IMPROVEMENTS.md, item 4.8a). A backend may deliver a long
// message in pieces; this puts them back together, following the MIDI
// specification:
//
// - F0 starts a message. A new F0 inside a message restarts it: the
//   unfinished one is aborted.
// - F7 ends it: the message, F0 to F7, comes out.
// - Real-time bytes (F8 to FF: clock, active sensing, ...) may come inside
//   a message. They are not part of it, and don't end it: they are skipped.
// - Any other status byte (80 to F6) aborts the message.
// - A message longer than the limit is dropped, so a source that sends F0
//   and no F7 can't make the buffer grow without bound. The rest of it, up
//   to the next F0 or status byte, is skipped.
// - Outside a message, everything but F0 is ignored: channel messages are
//   not this class's business.
//
// One assembler per source: bytes of two sources must never be mixed.
class SysexAssembler
{
public:
    // Far above an LPD8 message (66 bytes), and far below what could hurt.
    static constexpr qsizetype default_max_size = 4096;

    struct Result
    {
        QList<QByteArray> messages; // complete, F0 to F7
        int aborted{0}; // unfinished messages cut by F0 or a status byte
        // True the first time a message is dropped for its size, and not
        // again until a message completes: report it once, not once per
        // byte or per runaway message.
        bool tooLongReported{false};
    };

    explicit SysexAssembler(qsizetype maxSize = default_max_size);

    Result feed(QByteArrayView bytes);

    // The bytes of the unfinished message held now: never above the limit.
    qsizetype buffered() const { return m_buffer.size(); }

private:
    enum class State { Idle, InMessage, Skipping };

    void feedByte(char c, Result &result);

    qsizetype m_maxSize;
    QByteArray m_buffer;
    State m_state{State::Idle};
    bool m_tooLongReported{false};
};

} // namespace lpd8

#endif // SYSEXASSEMBLER_H
