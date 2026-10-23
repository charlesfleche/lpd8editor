#include "sysexhandler.h"

#include "lpd8_sysex.h"
#include "miditransport.h"

SysexHandler::SysexHandler(lpd8::MidiTransport &transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    connect(&m_transport, &lpd8::MidiTransport::sysexReceived, this, &SysexHandler::handleSysex);
}

void SysexHandler::getProgram(int id) const
{
    // XXX This check should be data driven, not hardcoded
    Q_ASSERT(id >= 1 && id <= 4);

    m_transport.sendSysex(sysex::getProgram(id));
}

void SysexHandler::sendProgram(const QByteArray &sysex, int deviceProgramId) const
{
    // XXX This check should be data driven, not hardcoded
    Q_ASSERT(deviceProgramId >= 1 && deviceProgramId <= 4);

    QByteArray s(sysex);
    sysex::makeSetProgramRequest(s, deviceProgramId);
    m_transport.sendSysex(s);
}

void SysexHandler::handleSysex(const QByteArray &sysex) const
{
    if (sysex::type(sysex) != sysex::TypeProgram) {
        return;
    }

    emit programReceived(sysex);
}
