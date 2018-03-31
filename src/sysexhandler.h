#ifndef SYSEXHANDLER_H
#define SYSEXHANDLER_H

#include <QObject>

#include <asoundlib.h>

class IOMidi;

class SysexHandler : public QObject
{
    Q_OBJECT
public:
    explicit SysexHandler(IOMidi* io);

    void getProgram(int deviceProgramId) const;
    void sendProgram(const QByteArray &sysex, int deviceProgramId) const;

signals:
    void programReceived(QByteArray) const;

public slots:
    void handleMidiEvent(const snd_seq_event_t* ev) const;


private:
    void sendSysex(const QByteArray &sysex) const;

    IOMidi *m_io;
};

#endif // SYSEXHANDLER_H
