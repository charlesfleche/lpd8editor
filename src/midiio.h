#ifndef MIDIIO_H
#define MIDIIO_H

#include "lpd8_types.h"

#include <QObject>

#include <asoundlib.h>

class MidiPortsModel;

class QAbstractItemModel;

class MidiIO : public QObject
{
    Q_OBJECT
public:
    explicit MidiIO(QObject *parent = 0);
    ~MidiIO();

    void sendSysex(QByteArray) const;
    void getProgram(int) const;
    void getPrograms() const;
    void sendProgram(pProgram) const;

    QAbstractItemModel* midiPortsModel();

signals:
    void sysexReceived(QByteArray);
    void programReceived(pProgram);

private slots:
    void readEvents();
    void processEvent(snd_seq_event_t *);

private:
    void processPortSubscribed(snd_seq_event_t*);
    void processSysex(snd_seq_event_t*);

    void sendIdRequest() const;

    snd_seq_t* m_seq_handle;
    struct pollfd* m_pfds;
    int m_seq_port;
    MidiPortsModel* m_ports_model;
};

#endif // MIDIIO_H
