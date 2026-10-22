#include "iomidi.h"

#include <QCoreApplication>
#include <QSocketNotifier>

#include <QtDebug>

#include <cerrno>
#include <vector>

IOMidi::IOMidi(QObject *parent)
    : QObject(parent)
{
    qDebug() << "Opening MIDI sequencer";

    snd_seq_t *seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        throw std::runtime_error("Failed opening sequencer");
    }
    // From here on, m_seq owns the handle: any later throw closes it.
    m_seq.reset(seq);

    // Poll the sequencer's file descriptors from the Qt event loop. The
    // array is only needed while setting up the notifiers.

    const int count = snd_seq_poll_descriptors_count(handle(), POLLIN);
    std::vector<pollfd> pfds(static_cast<std::size_t>(count));
    const int filled = snd_seq_poll_descriptors(
            handle(), pfds.data(), static_cast<unsigned int>(pfds.size()), POLLIN);
    pfds.resize(static_cast<std::size_t>(filled));

    for (const pollfd &pfd : pfds) {
        qDebug() << "Enabling MIDI poll descriptor" << pfd.fd;
        auto *n = new QSocketNotifier(pfd.fd, QSocketNotifier::Read, this);
        // Context 'n': if the notifier is destroyed first (the IOMidi is deleted
        // before the application quits), the connection goes with it.
        connect(QCoreApplication::instance(),
                &QCoreApplication::aboutToQuit,
                n,
                [fd = pfd.fd, n]() {
                    qDebug() << "Disabling MIDI poll descriptor" << fd;
                    n->setEnabled(false);
                });
        connect(n, &QSocketNotifier::activated, this, &IOMidi::readEvents);
    }

    if (snd_seq_set_client_name(handle(), qPrintable(QCoreApplication::applicationName())) < 0) {
        throw std::runtime_error("Failed setting sequencer name");
    }

    const int res = snd_seq_create_simple_port(handle(),
                                               qPrintable(QCoreApplication::applicationName()),
                                               SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE
                                                       | SND_SEQ_PORT_CAP_SUBS_READ
                                                       | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                               SND_SEQ_PORT_TYPE_APPLICATION);
    if (res < 0) {
        throw std::runtime_error("Failed setting sequencer port");
    }
    m_seq_port = static_cast<unsigned char>(res);

    // Connect to announce port

    if (snd_seq_connect_from(
                handle(), m_seq_port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE)
        < 0) {
        throw std::runtime_error("Cannot connect to announce port");
    }
}

IOMidi::~IOMidi()
{
    // m_seq then closes the handle.
    qDebug() << "Closing MIDI sequencer";
    if (snd_seq_delete_simple_port(handle(), m_seq_port) < 0) {
        qWarning() << "Failed deleting sequencer port";
    }
}

void IOMidi::SequencerCloser::operator()(snd_seq_t *seq) const
{
    if (snd_seq_close(seq) < 0) {
        qWarning() << "Failed closing sequencer";
    }
}

snd_seq_t *IOMidi::handle() const
{
    return m_seq.get();
}

unsigned char IOMidi::clientId() const
{
    // ALSA client ids are below 256 (SND_SEQ_MAX_CLIENTS is 192)
    return static_cast<unsigned char>(snd_seq_client_id(handle()));
}

unsigned char IOMidi::portId() const
{
    return m_seq_port;
}

void IOMidi::readEvents() const
{
    while (snd_seq_event_input_pending(handle(), 1) > 0) {
        snd_seq_event_t *ev = nullptr;
        const int res = snd_seq_event_input(handle(), &ev);
        if (res == -ENOSPC) {
            // The input buffer overflowed and events were lost. ALSA has
            // reset the buffer, so keep reading what came after.
            qWarning() << "MIDI input overrun: some events were lost";
            continue;
        }
        if (res < 0 || ev == nullptr) {
            // -EAGAIN (nothing left after all) or a real error: 'ev' was not
            // set, so there is nothing to emit.
            if (res != -EAGAIN) {
                qWarning() << "Failed reading a MIDI event:" << snd_strerror(res);
            }
            return;
        }
        emit eventReceived(ev);
    }
}

int IOMidi::sendEvent(snd_seq_event_t *ev) const
{
    Q_ASSERT(ev);

    snd_seq_ev_set_source(ev, m_seq_port);
    const int ret = snd_seq_event_output_direct(handle(), ev);
    if (ret < 0) {
        qWarning() << "Failed to send sysex";
    }
    return ret;
}
