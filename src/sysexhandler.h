#ifndef SYSEXHANDLER_H
#define SYSEXHANDLER_H

#include <QObject>

namespace lpd8 {
class MidiTransport;
}

// The LPD8 requests of the MIDI menu, over a MidiTransport. Replaced by
// Lpd8Device in item 4.10.
class SysexHandler : public QObject
{
    Q_OBJECT
public:
    // 'transport' must outlive the handler.
    explicit SysexHandler(lpd8::MidiTransport &transport, QObject *parent = nullptr);

    void getProgram(int deviceProgramId) const;
    void sendProgram(const QByteArray &sysex, int deviceProgramId) const;

signals:
    // A program dump, from any source: checking it is item 4.10's job.
    void programReceived(QByteArray) const;

private:
    void handleSysex(const QByteArray &sysex) const;

    lpd8::MidiTransport &m_transport;
};

#endif // SYSEXHANDLER_H
