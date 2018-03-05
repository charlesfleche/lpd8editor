#include "midiio.h"

#include "lpd8_sysex.h"
#include "midiportsmodel.h"

#include <QGuiApplication>
#include <QSocketNotifier>
#include <QtDebug>
#include <QThread>

#include <exception>

MidiIO::MidiIO(QObject *parent) : QObject(parent),
    m_seq_handle(Q_NULLPTR),
    m_pfds(Q_NULLPTR),
    m_seq_port(-1),
    m_ports_model(Q_NULLPTR)
{
    qDebug() << "Opening MIDI sequencer";

    if (snd_seq_open(&m_seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        throw std::runtime_error("Failed opening sequencer");
    }

    // Poll

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

    //    snd_seq_set_client_event_filter(m_seq_handle, SND_SEQ_EVENT_SYSEX);

    // Try qUtf8Printable
    if (snd_seq_set_client_name(m_seq_handle, qPrintable(qApp->applicationName())) < 0) {
        throw std::runtime_error("Failed setting sequencer name");
    }

    // In port
    m_seq_port = snd_seq_create_simple_port(
                m_seq_handle,
                qPrintable(qGuiApp->applicationName()),
                SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_DUPLEX | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_SUBS_WRITE,
                SND_SEQ_PORT_TYPE_APPLICATION);
    if (m_seq_port < 0) {
        throw std::runtime_error("Failed setting sequencer port");
    }

    m_ports_model = new MidiPortsModel(m_seq_handle, m_seq_port, this);
}

MidiIO::~MidiIO() {
    qDebug() << "Closing MIDI sequencer";
    if (snd_seq_delete_simple_port(m_seq_handle, m_seq_port) >= 0) {
        m_seq_port = -1;
    } else {
        qWarning() << "Failed deleting sequencer port";
    }

    if (snd_seq_close(m_seq_handle) >= 0) {
        m_seq_handle = Q_NULLPTR;
    } else {
        qWarning() << "Failed closing sequencer";
    }
}

void MidiIO::readEvents()
{
    qDebug();
    while (snd_seq_event_input_pending(m_seq_handle, 1) > 0) {
        snd_seq_event_t *ev;
        snd_seq_event_input(m_seq_handle, &ev);
        processEvent(ev);
    }
}

void MidiIO::processEvent(snd_seq_event_t* ev)
{
    switch (ev->type) {
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
        processPortSubscribed(ev);
        break;
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
        break;
    case SND_SEQ_EVENT_PORT_START:
        break;
    case SND_SEQ_EVENT_PORT_EXIT:
        break;
    case SND_SEQ_EVENT_PORT_CHANGE:
        break;
    case SND_SEQ_EVENT_SYSEX:
        processSysex(ev);
    default:
        break;
    }
}

void MidiIO::processPortSubscribed(snd_seq_event_t* ev) {
    Q_CHECK_PTR(ev);
    Q_ASSERT(ev->type == SND_SEQ_EVENT_PORT_SUBSCRIBED);

//    sendIdRequest();
}

void MidiIO::processSysex(snd_seq_event_t* ev) {
    Q_CHECK_PTR(ev);
    Q_ASSERT(ev->type == SND_SEQ_EVENT_SYSEX);

    QByteArray s(
        static_cast<const char*>(ev->data.ext.ptr),
        ev->data.ext.len);

    switch (sysex::type(s)) {
    case sysex::TypeProgram:
        emit programReceived(sysex::toProgram(s));
        break;
    default:
        break;
    }
    emit sysexReceived(s);
}

void MidiIO::sendIdRequest() const
{
    const unsigned char b[] = {0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7};
    const QByteArray sysex(reinterpret_cast<const char*>(b), sizeof(b));
    sendSysex(sysex);
}

void MidiIO::sendSysex(QByteArray sysex) const
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_source(&ev, m_seq_port);
    snd_seq_ev_set_sysex(&ev, sysex.length(), sysex.data());

    int err = snd_seq_event_output(m_seq_handle, &ev);
    if (err < 0) {
        throw std::runtime_error(snd_strerror(err));
    }
    err = 1;
    while (err > 0) {
        err = snd_seq_drain_output(m_seq_handle);
        if (err < 0) {
            throw std::runtime_error(snd_strerror(err));
        }
    }
}

void MidiIO::getPrograms() const {
    for (int i = 1 ; i <= 4 ; ++i) {
        sendSysex(sysex::getProgram(i));
    }
}

void MidiIO::sendProgram(pProgram program) const
{
    QByteArray s = sysex::setProgram(program);
    sendSysex(s);
}

void MidiIO::getProgram(int id) const {
    Q_ASSERT(id >= 1 && id <= 4);

    sendSysex(sysex::getProgram(id));
    QThread::sleep(1);
}

QAbstractItemModel* MidiIO::midiPortsModel() {
    Q_CHECK_PTR(m_ports_model);

    return m_ports_model;
}
