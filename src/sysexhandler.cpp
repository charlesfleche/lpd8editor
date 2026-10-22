#include "sysexhandler.h"

#include "iomidi.h"
#include "lpd8_sysex.h"

SysexHandler::SysexHandler(IOMidi *io)
    : QObject(io)
    , m_io(io)
{
    Q_ASSERT(m_io);

    connect(m_io, &IOMidi::eventReceived, this, &SysexHandler::handleMidiEvent);
}

void SysexHandler::getProgram(int id) const
{
    // XXX This check should be data driven, not hardcoded
    Q_ASSERT(id >= 1 && id <= 4);

    sendSysex(sysex::getProgram(id));
}

void SysexHandler::sendProgram(const QByteArray &sysex, int deviceProgramId) const
{
    // XXX This check should be data driven, not hardcoded
    Q_ASSERT(deviceProgramId >= 1 && deviceProgramId <= 4);

    QByteArray s(sysex);
    sysex::makeSetProgramRequest(s, deviceProgramId);
    sendSysex(s);
}

void SysexHandler::handleMidiEvent(const snd_seq_event_t *ev) const
{
    Q_ASSERT(ev);

    if (ev->type != SND_SEQ_EVENT_SYSEX) {
        return;
    }

    const QByteArray s(static_cast<const char *>(ev->data.ext.ptr), ev->data.ext.len);

    if (sysex::type(s) != sysex::TypeProgram) {
        return;
    }

    emit programReceived(s);
}

void SysexHandler::sendSysex(const QByteArray &sysex) const
{
    Q_ASSERT(!sysex.isEmpty());

    QByteArray s(sysex);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    // The ALSA macro clears a flag with '&= ~MASK' on an unsigned char,
    // which GCC reports as a sign conversion. The warning is about the
    // macro's code, not ours, so it is silenced for this line only.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    snd_seq_ev_set_sysex(&ev, static_cast<unsigned int>(s.length()), s.data());
#pragma GCC diagnostic pop

    m_io->sendEvent(&ev);
}
