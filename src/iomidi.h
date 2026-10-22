#ifndef IOMIDI_H
#define IOMIDI_H

#include <QObject>

#include <alsa/asoundlib.h>

class IOMidi : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(IOMidi)

public:
    explicit IOMidi(QObject *parent = nullptr);
    ~IOMidi() override;

    snd_seq_t *handle() const;
    unsigned char clientId() const;
    unsigned char portId() const;

    int sendEvent(snd_seq_event_t *ev) const;

signals:
    void eventReceived(snd_seq_event_t *ev) const;

private:
    void readEvents() const;

    snd_seq_t *m_seq_handle{nullptr};
    unsigned char m_seq_port{0}; // only read after the constructor has set it
    struct pollfd *m_pfds{nullptr};
};

#endif // IOMIDI_H
