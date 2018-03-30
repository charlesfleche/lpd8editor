#ifndef IOMIDI_H
#define IOMIDI_H

#include <QObject>

#include <asoundlib.h>

class IOMidi : public QObject
{
    Q_OBJECT
public:
    explicit IOMidi(QObject *parent = nullptr);
    virtual ~IOMidi();

    snd_seq_t* handle() const;
    unsigned char clientId() const;
    unsigned char portId() const;

    int sendEvent(snd_seq_event_t *ev) const;

signals:
    void eventReceived(snd_seq_event_t *ev) const;

private:
    void readEvents() const;

    snd_seq_t* m_seq_handle;
    unsigned char m_seq_port;
    struct pollfd* m_pfds;
};

#endif // IOMIDI_H
