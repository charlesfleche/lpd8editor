#include "sysexhandler.h"

#include "iomidi.h"
#include "lpd8_sysex.h"

SysexHandler::SysexHandler(IOMidi *io) :
    QObject(io),
    m_io(io)
{
    Q_CHECK_PTR(m_io);

    connect(
        m_io,
        &IOMidi::eventReceived,
        this,
        &SysexHandler::handleMidiEvent
    );
}

void SysexHandler::getProgram(int id) const {
    // XXX This check should be data driven, not hardcoded
    Q_ASSERT(id >= 1 && id <= 4);

    sendSysex(sysex::getProgram(id));
}

void SysexHandler::sendProgram(const QByteArray &sysex, int deviceProgramId) const {
    // XXX This check should be data driven, not hardcoded
    Q_ASSERT(deviceProgramId >= 1 && deviceProgramId <= 4);
    Q_CHECK_PTR(m_io);

    QByteArray s(sysex);
    sysex::makeSetProgramRequest(s, deviceProgramId);
    sendSysex(s);
}

void SysexHandler::handleMidiEvent(const snd_seq_event_t* ev) const {
    Q_CHECK_PTR(ev);

    if (ev->type != SND_SEQ_EVENT_SYSEX) {
        return;
    }

    QByteArray s(
        static_cast<const char*>(ev->data.ext.ptr),
        ev->data.ext.len);

    if (sysex::type(s) != sysex::TypeProgram) {
        return;
    }

    emit programReceived(s);
}

void SysexHandler::sendSysex(const QByteArray &sysex) const {
    Q_ASSERT(!sysex.isEmpty());

    QByteArray s(sysex);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_sysex(&ev, s.length(), s.data());

    m_io->sendEvent(&ev);
}
