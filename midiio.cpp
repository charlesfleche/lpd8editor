#include "midiio.h"

#include<QGuiApplication>
#include <QSocketNotifier>
#include <QtDebug>

#include <exception>

MidiIO::MidiIO(QObject *parent) : QObject(parent),
    m_seq_handle(Q_NULLPTR),
    m_pfds(Q_NULLPTR),
    m_seq_port_in(-1),
    m_seq_port_out(-1)
{
    qDebug() << "Opening MIDI sequencer";

    if (snd_seq_open(&m_seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        throw std::runtime_error("Failed opening sequencer");
    }

    // Try qUtf8Printable
    if (snd_seq_set_client_name(m_seq_handle, qPrintable(qApp->applicationName())) < 0) {
        throw std::runtime_error("Failed setting sequencer name");
    }

    // In port
    m_seq_port_in = snd_seq_create_simple_port(
                m_seq_handle,
                qPrintable(qGuiApp->applicationName() + ":in"),
                SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                SND_SEQ_PORT_TYPE_APPLICATION);
    if (m_seq_port_in < 0) {
        throw std::runtime_error("Failed setting sequencer port IN");
    }

    // Out port
    m_seq_port_out = snd_seq_create_simple_port(
                m_seq_handle,
                qPrintable(qGuiApp->applicationName() + ":out"),
                SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                SND_SEQ_PORT_TYPE_APPLICATION);
    if (m_seq_port_out < 0) {
        throw std::runtime_error("Failed setting sequencer port OUT");
    }

    int npfd = snd_seq_poll_descriptors_count(m_seq_handle, POLLIN);
    m_pfds = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    int npfdf = snd_seq_poll_descriptors(m_seq_handle, m_pfds, npfd, POLLIN);

    for (int i = 0 ; i < npfdf ; ++i) {
        qDebug() << "Enabling MIDI poll descriptor" << i;
        QSocketNotifier* n = new QSocketNotifier(m_pfds[i].fd, QSocketNotifier::Read);
        connect(qGuiApp,
                &QGuiApplication::aboutToQuit,
                [=]() {
                    qDebug() << "Disabling MIDI poll descriptor" << i;
                    n->setEnabled(false);
                }
        );
        connect(n,
                &QSocketNotifier::activated,
                this,
                &MidiIO::readEvents
        );
    }
}

MidiIO::~MidiIO() {
    qDebug() << "Closing MIDI sequencer";
    if (snd_seq_delete_simple_port(m_seq_handle, m_seq_port_in) < 0) {
        qWarning() << "Failed deleting sequencer port IN";
    }

    if (snd_seq_delete_simple_port(m_seq_handle, m_seq_port_out) < 0) {
        qWarning() << "Failed deleting sequencer port OUT";
    }

    if (snd_seq_close(m_seq_handle) < 0) {
        qWarning() << "Failed closing sequencer";
    }
}

void MidiIO::readEvents() const {
    int r = 1;
    while (r>0) {
        snd_seq_event_t *ev;
        r = snd_seq_event_input(m_seq_handle, &ev);
        emit eventReceived(ev);
    }
}
