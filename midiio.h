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

signals:
    void eventReceived(snd_seq_event_t *) const;

private slots:
    void readEvents() const;

private:
    snd_seq_t* m_seq_handle;
    struct pollfd * m_pfds;
    int m_seq_port_in;
    int m_seq_port_out;
};

#endif // MIDIIO_H
