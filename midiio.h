#ifndef MIDIIO_H
#define MIDIIO_H

#include <QObject>

#include <asoundlib.h>

class MidiIO : public QObject
{
    Q_OBJECT
public:
    explicit MidiIO(QObject *parent = 0);
    ~MidiIO();

    void sendSysex(QByteArray);

signals:
    void sysexReceived(QByteArray);

private slots:
    void readEvents();
    void processEvent(snd_seq_event_t *);

private:
    void processPortSubscribed(snd_seq_event_t*);
    void processSysex(snd_seq_event_t*);

    void sendIdRequest();

    snd_seq_t* m_seq_handle;
    struct pollfd * m_pfds;
    int m_seq_port_in;
    int m_seq_port_out;
};

#endif // MIDIIO_H
