#include "iomidi.h"

#include <QApplication>
#include <QSocketNotifier>

#include <QtDebug>

IOMidi::IOMidi(QObject *parent) :
    QObject(parent),
    m_seq_handle(Q_NULLPTR),
    m_seq_port(-1),
    m_pfds(Q_NULLPTR)
{
    qDebug() << "Opening MIDI sequencer";

    if (snd_seq_open(&m_seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        throw std::runtime_error("Failed opening sequencer");
    }

    // Poll

    int npfd = snd_seq_poll_descriptors_count(m_seq_handle, POLLIN);
    m_pfds = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    int npfdf = snd_seq_poll_descriptors(m_seq_handle, m_pfds, npfd, POLLIN);

    for (int i = 0 ; i < npfdf ; ++i) {
        qDebug() << "Enabling MIDI poll descriptor" << i;
        QSocketNotifier* n = new QSocketNotifier(m_pfds[i].fd, QSocketNotifier::Read, this);
        connect(qApp,
                &QApplication::aboutToQuit,
                [=]() {
                    qDebug() << "Disabling MIDI poll descriptor" << i;
                    n->setEnabled(false);
                }
        );
        connect(n,
                &QSocketNotifier::activated,
                this,
                &IOMidi::readEvents
        );
    }

    if (snd_seq_set_client_name(m_seq_handle, qPrintable(qApp->applicationName())) < 0) {
        throw std::runtime_error("Failed setting sequencer name");
    }

    const int res = snd_seq_create_simple_port(
                m_seq_handle,
                qPrintable(qGuiApp->applicationName()),
                SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE |
                SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_SUBS_WRITE,
                SND_SEQ_PORT_TYPE_APPLICATION);
    if (res < 0) {
        throw std::runtime_error("Failed setting sequencer port");
    } else {
        m_seq_port = static_cast<unsigned char>(res);
    }

    // Connect to announce port

    if (snd_seq_connect_from(m_seq_handle, m_seq_port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE) < 0) {
        throw std::runtime_error("Cannot connect to announce port");
    }
}

IOMidi::~IOMidi() {
    qDebug() << "Closing MIDI sequencer";
    if (snd_seq_delete_simple_port(m_seq_handle, m_seq_port) < 0) {
        qWarning() << "Failed deleting sequencer port";
    }

    if (snd_seq_close(m_seq_handle) < 0) {
        qWarning() << "Failed closing sequencer";
    }
}

snd_seq_t* IOMidi::handle() const {
    return m_seq_handle;
}

unsigned char IOMidi::clientId() const {
    return snd_seq_client_id(m_seq_handle);
}

unsigned char IOMidi::portId() const {
    return m_seq_port;
}

void IOMidi::readEvents() const
{
    while (snd_seq_event_input_pending(m_seq_handle, 1) > 0) {
        snd_seq_event_t *ev;
        snd_seq_event_input(m_seq_handle, &ev);
        emit eventReceived(ev);
    }
}

int IOMidi::sendEvent(snd_seq_event_t *ev) const
{
    Q_CHECK_PTR(ev);

    snd_seq_ev_set_source(ev, m_seq_port);
    return snd_seq_event_output_direct(m_seq_handle, ev);
}
