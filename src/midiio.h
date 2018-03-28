#ifndef MIDIIO_H
#define MIDIIO_H

#include "lpd8_types.h"

#include <QObject>

#include <asoundlib.h>

class QAbstractItemModel;
class QStandardItemModel;

class MidiIO : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(bool canSelectDevice READ canSelectDevice NOTIFY canSelectDeviceChanged)

    explicit MidiIO(QObject *parent = 0);
    ~MidiIO();

    void sendSysex(const QByteArray&) const;
    void getProgram(int) const;
    void getPrograms() const;
    void sendProgram(pProgram) const;

    void sendProgramSysex(const QByteArray &sysex, int programId) const;

    bool canSelectDevice() const;
    QAbstractItemModel* midiPortsModel() const;

public slots:
    void rescanPorts();
    void connectPort(const QModelIndex&);

signals:
    void sysexReceived(QByteArray);
    void programReceived(QByteArray);
    void canSelectDeviceChanged(bool);

private slots:
    void disallowManualConnections();
    void readEvents();
    void processEvent(snd_seq_event_t *);

private:
    void processSysex(snd_seq_event_t*);

    void sendIdRequest() const;

    snd_seq_t* m_seq_handle;
    struct pollfd* m_pfds;
    int m_seq_port;
    QStandardItemModel* m_ports_model;
};

#endif // MIDIIO_H
