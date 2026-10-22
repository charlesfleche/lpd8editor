#ifndef IOMIDI_H
#define IOMIDI_H

#include <QObject>

#include <alsa/asoundlib.h>

#include <memory>
#include <vector>

class QSocketNotifier;

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

    // Closes the sequencer handle. As a unique_ptr deleter, it runs when the
    // IOMidi is destroyed, and also when the constructor throws after the
    // handle was opened: members that are already constructed are destroyed
    // then, while the destructor of a partly constructed object never runs.
    struct SequencerCloser
    {
        void operator()(snd_seq_t *seq) const;
    };

    std::unique_ptr<snd_seq_t, SequencerCloser> m_seq;
    unsigned char m_seq_port{0}; // only read after the constructor has set it

    // Watch the sequencer's file descriptors. Declared *after* m_seq on
    // purpose: members are destroyed in the reverse order of their
    // declaration, so the notifiers go before the handle closes their file
    // descriptors, in the destructor and on every throw path of the
    // constructor. As QObject children they would only be deleted by
    // ~QObject, after m_seq: watching closed descriptors whose numbers can
    // be reused in the meantime. Having no parent, they would not follow
    // an IOMidi::moveToThread(): they would have to be moved one by one.
    std::vector<std::unique_ptr<QSocketNotifier>> m_notifiers;
};

#endif // IOMIDI_H
