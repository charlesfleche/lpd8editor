#include "alsaseqtransport.h"

#include "sysexassembler.h"

#include <QCoreApplication>
#include <QHash>
#include <QSocketNotifier>

#include <QtDebug>

#include <alsa/asoundlib.h>

#include <cerrno>
#include <stdexcept>
#include <vector>

// 'static' rather than an anonymous namespace: QList::indexOf() finds this
// operator through argument-dependent lookup, which only searches the
// namespace of snd_seq_addr_t (the global one). A function in an anonymous
// namespace is not found that way, and the build fails.
// NOLINTNEXTLINE(misc-use-anonymous-namespace)
static bool operator==(const snd_seq_addr_t &lhs, const snd_seq_addr_t &rhs)
{
    return lhs.client == rhs.client && lhs.port == rhs.port;
}

namespace lpd8 {

namespace {

QString idOf(const snd_seq_addr_t &addr)
{
    return QStringLiteral("%1:%2").arg(QString::number(addr.client), QString::number(addr.port));
}

} // namespace

struct AlsaSeqTransport::Private
{
    explicit Private(AlsaSeqTransport *transport)
        : q(transport)
    { }

    // Closes the sequencer handle. As a unique_ptr deleter, it runs when the
    // transport is destroyed, and also when the constructor throws after the
    // handle was opened: members that are already constructed are destroyed
    // then, while the destructor of a partly constructed object never runs.
    struct SequencerCloser
    {
        void operator()(snd_seq_t *handle) const
        {
            if (snd_seq_close(handle) < 0) {
                qWarning() << "Failed closing sequencer";
            }
        }
    };

    snd_seq_t *handle() const { return seq.get(); }
    snd_seq_addr_t ownAddr() const
    {
        // ALSA client ids are below 256 (SND_SEQ_MAX_CLIENTS is 192)
        return {.client = static_cast<unsigned char>(snd_seq_client_id(handle())), .port = port};
    }

    // What one event means for the transport: a SysEx piece, or a port or
    // subscription announcement. Emits the transport's signals (Qt signals
    // are public), through 'q'.
    void handle(const snd_seq_event_t &ev);
    bool acceptAddress(const snd_seq_addr_t &addr) const;
    MidiPort::Directions connection(const snd_seq_addr_t &addr) const;
    void listPorts();
    void disconnectAll() const;

    AlsaSeqTransport *q; // the transport, whose signals handle() emits
    std::unique_ptr<snd_seq_t, SequencerCloser> seq;
    unsigned char port{0}; // only read after the constructor has set it

    // Watch the sequencer's file descriptors. Declared *after* seq on
    // purpose: members are destroyed in the reverse order of their
    // declaration, so the notifiers go before the handle closes their file
    // descriptors, in the destructor and on every throw path of the
    // constructor. As QObject children they would only be deleted by
    // ~QObject, after seq: watching closed descriptors whose numbers can
    // be reused in the meantime. Having no parent, they would not follow
    // a moveToThread(): they would have to be moved one by one.
    std::vector<std::unique_ptr<QSocketNotifier>> notifiers;

    // The ports listed, in the order they appeared.
    QList<snd_seq_addr_t> addrs;
    // One per source: the bytes of two sources must never be mixed.
    QHash<QString, SysexAssembler> assemblers;
};

AlsaSeqTransport::AlsaSeqTransport(QObject *parent)
    : MidiTransport(parent)
    , d(std::make_unique<Private>(this))
{
    qDebug() << "Opening MIDI sequencer";

    snd_seq_t *seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        throw std::runtime_error("Failed opening sequencer");
    }
    // From here on, d->seq owns the handle: any later throw closes it.
    d->seq.reset(seq);

    // Poll the sequencer's file descriptors from the Qt event loop. The
    // array is only needed while setting up the notifiers.

    const int count = snd_seq_poll_descriptors_count(d->handle(), POLLIN);
    std::vector<pollfd> pfds(static_cast<std::size_t>(count));
    const int filled = snd_seq_poll_descriptors(
            d->handle(), pfds.data(), static_cast<unsigned int>(pfds.size()), POLLIN);
    pfds.resize(static_cast<std::size_t>(filled));

    for (const pollfd &pfd : pfds) {
        qDebug() << "Enabling MIDI poll descriptor" << pfd.fd;
        auto notifier = std::make_unique<QSocketNotifier>(pfd.fd, QSocketNotifier::Read);
        QSocketNotifier *n = notifier.get();
        d->notifiers.push_back(std::move(notifier));
        // Context 'n': if the notifier is destroyed first (the transport is
        // deleted before the application quits), the connection goes with it.
        connect(QCoreApplication::instance(),
                &QCoreApplication::aboutToQuit,
                n,
                [fd = pfd.fd, n]() {
                    qDebug() << "Disabling MIDI poll descriptor" << fd;
                    n->setEnabled(false);
                });
        connect(n, &QSocketNotifier::activated, this, &AlsaSeqTransport::readEvents);
    }

    if (snd_seq_set_client_name(d->handle(), qPrintable(QCoreApplication::applicationName())) < 0) {
        throw std::runtime_error("Failed setting sequencer name");
    }

    const int res = snd_seq_create_simple_port(d->handle(),
                                               qPrintable(QCoreApplication::applicationName()),
                                               SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE
                                                       | SND_SEQ_PORT_CAP_SUBS_READ
                                                       | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                               SND_SEQ_PORT_TYPE_APPLICATION);
    if (res < 0) {
        throw std::runtime_error("Failed setting sequencer port");
    }
    d->port = static_cast<unsigned char>(res);

    // Connect to announce port: ports and subscriptions coming and going.

    if (snd_seq_connect_from(
                d->handle(), d->port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE)
        < 0) {
        throw std::runtime_error("Cannot connect to announce port");
    }

    d->listPorts();
}

AlsaSeqTransport::~AlsaSeqTransport()
{
    // d then closes the notifiers, and the handle.
    qDebug() << "Closing MIDI sequencer";
    if (snd_seq_delete_simple_port(d->handle(), d->port) < 0) {
        qWarning() << "Failed deleting sequencer port";
    }
}

QString AlsaSeqTransport::ownPortId() const
{
    return idOf(d->ownAddr());
}

bool AlsaSeqTransport::Private::acceptAddress(const snd_seq_addr_t &addr) const
{
    // Not the application itself, nor the system client (timer, announce).
    if (addr.client == ownAddr().client || addr.client == SND_SEQ_CLIENT_SYSTEM) {
        return false;
    }
    snd_seq_port_info_t *pinfo = nullptr;
    snd_seq_port_info_alloca(&pinfo);
    if (snd_seq_get_any_port_info(handle(), addr.client, addr.port, pinfo) < 0) {
        return false;
    }
    const unsigned int caps = snd_seq_port_info_get_capability(pinfo);
    return (caps & SND_SEQ_PORT_CAP_NO_EXPORT) == 0 && (caps & SND_SEQ_PORT_CAP_READ) != 0
            && (caps & SND_SEQ_PORT_CAP_WRITE) != 0;
}

void AlsaSeqTransport::Private::listPorts()
{
    // The clients and ports already there. Later ones come as announcements.
    snd_seq_client_info_t *cinfo = nullptr;
    snd_seq_client_info_alloca(&cinfo);

    snd_seq_port_info_t *pinfo = nullptr;
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(handle(), cinfo) >= 0) {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(handle(), pinfo) >= 0) {
            const snd_seq_addr_t addr = *snd_seq_port_info_get_addr(pinfo);
            if (acceptAddress(addr)) {
                addrs.append(addr);
            }
        }
    }
}

MidiPort::Directions AlsaSeqTransport::Private::connection(const snd_seq_addr_t &addr) const
{
    MidiPort::Directions ret = MidiPort::Disconnected;

    const snd_seq_addr_t own = ownAddr();

    snd_seq_port_subscribe_t *subs = nullptr;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_port_subscribe_set_sender(subs, &own);
    snd_seq_port_subscribe_set_dest(subs, &addr);
    if (snd_seq_get_port_subscription(handle(), subs) == 0) {
        ret |= MidiPort::To;
    }

    snd_seq_port_subscribe_set_sender(subs, &addr);
    snd_seq_port_subscribe_set_dest(subs, &own);
    if (snd_seq_get_port_subscription(handle(), subs) == 0) {
        ret |= MidiPort::From;
    }

    return ret;
}

QList<MidiPort> AlsaSeqTransport::ports() const
{
    QList<MidiPort> result;
    snd_seq_port_info_t *info = nullptr;
    snd_seq_port_info_alloca(&info);
    for (const snd_seq_addr_t &addr : d->addrs) {
        QString name;
        if (snd_seq_get_any_port_info(d->handle(), addr.client, addr.port, info) >= 0) {
            name = QString::fromUtf8(snd_seq_port_info_get_name(info));
        }
        result.append(MidiPort{.id = idOf(addr), .name = name, .connection = d->connection(addr)});
    }
    return result;
}

void AlsaSeqTransport::Private::disconnectAll() const
{
    const snd_seq_addr_t own = ownAddr();

    snd_seq_query_subscribe_t *subs = nullptr;
    snd_seq_query_subscribe_alloca(&subs);
    snd_seq_query_subscribe_set_root(subs, &own);

    // Both directions. The announce subscription is not one of the
    // application's connections: it stays.
    for (const auto type : {SND_SEQ_QUERY_SUBS_READ, SND_SEQ_QUERY_SUBS_WRITE}) {
        snd_seq_query_subscribe_set_index(subs, 0);
        snd_seq_query_subscribe_set_type(subs, type);
        while (snd_seq_query_port_subscribers(handle(), subs) >= 0) {
            const snd_seq_addr_t addr = *snd_seq_query_subscribe_get_addr(subs);
            const bool announce = addr.client == SND_SEQ_CLIENT_SYSTEM
                    && addr.port == SND_SEQ_PORT_SYSTEM_ANNOUNCE;
            int res = -1;
            if (!announce) {
                res = type == SND_SEQ_QUERY_SUBS_READ
                        ? snd_seq_disconnect_to(handle(), port, addr.client, addr.port)
                        : snd_seq_disconnect_from(handle(), port, addr.client, addr.port);
            }
            if (res < 0) {
                // Not removed: the next subscriber is at the next index.
                // Removed: the next one has moved to this index.
                snd_seq_query_subscribe_set_index(subs,
                                                  snd_seq_query_subscribe_get_index(subs) + 1);
            }
        }
    }
}

bool AlsaSeqTransport::connectTo(const QString &portId)
{
    const auto it = std::ranges::find(d->addrs, portId, idOf);
    if (it == d->addrs.end()) {
        return false;
    }
    const snd_seq_addr_t addr = *it;
    d->disconnectAll();
    const bool ok = snd_seq_connect_from(d->handle(), d->port, addr.client, addr.port) >= 0
            && snd_seq_connect_to(d->handle(), d->port, addr.client, addr.port) >= 0;
    if (!ok) {
        qWarning() << "Failed to connect to" << portId;
    }
    // Now, not only when the announcement arrives: the window follows at
    // once.
    emit connectionChanged();
    return ok;
}

void AlsaSeqTransport::disconnectAll()
{
    d->disconnectAll();
    emit connectionChanged();
}

void AlsaSeqTransport::readEvents()
{
    while (snd_seq_event_input_pending(d->handle(), 1) > 0) {
        snd_seq_event_t *ev = nullptr;
        const int res = snd_seq_event_input(d->handle(), &ev);
        if (res == -ENOSPC) {
            // The input buffer overflowed and events were lost. ALSA has
            // reset the buffer, so keep reading what came after.
            qWarning() << "MIDI input overrun: some events were lost";
            continue;
        }
        if (res < 0 || ev == nullptr) {
            // -EAGAIN (nothing left after all) or a real error: 'ev' was not
            // set, so there is nothing to handle.
            if (res != -EAGAIN) {
                qWarning() << "Failed reading a MIDI event:" << snd_strerror(res);
            }
            return;
        }

        d->handle(*ev);
    }
}

void AlsaSeqTransport::Private::handle(const snd_seq_event_t &ev)
{
    switch (ev.type) {
    case SND_SEQ_EVENT_SYSEX: {
        // The event's data is only valid until the next input: it is
        // copied by the assembler, or into the messages it returns.
        const QString source = idOf(ev.source);
        const QByteArrayView bytes(static_cast<const char *>(ev.data.ext.ptr),
                                   static_cast<qsizetype>(ev.data.ext.len));
        const SysexAssembler::Result result = assemblers[source].feed(bytes);
        if (result.tooLongReported) {
            qWarning() << "Ignoring a SysEx message from" << source << "longer than"
                       << SysexAssembler::default_max_size << "bytes";
        }
        for (const QByteArray &message : result.messages) {
            emit q->sysexReceived(message, source);
        }
        break;
    }
    case SND_SEQ_EVENT_PORT_START:
        if (!addrs.contains(ev.data.addr) && acceptAddress(ev.data.addr)) {
            addrs.append(ev.data.addr);
            emit q->portsChanged();
        }
        break;
    case SND_SEQ_EVENT_PORT_EXIT:
        if (addrs.removeAll(ev.data.addr) > 0) {
            assemblers.remove(idOf(ev.data.addr));
            emit q->portsChanged();
            // Its connections are gone with it.
            emit q->connectionChanged();
        }
        break;
    case SND_SEQ_EVENT_PORT_CHANGE:
        if (addrs.contains(ev.data.addr)) {
            emit q->portsChanged(); // renamed, maybe
        }
        break;
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED: {
        const snd_seq_addr_t own = ownAddr();
        if (ev.data.connect.sender == own || ev.data.connect.dest == own) {
            emit q->connectionChanged();
        }
        break;
    }
    default:
        break;
    }
}

bool AlsaSeqTransport::sendCheckedSysex(QByteArrayView message)
{
    // snd_seq_event_output_direct() sends a variable-length event from a
    // temporary buffer, not through the client's output buffer, so the
    // output buffer size is not its limit. The receiving client's input
    // buffer is, and alsaseqtransport_test checks that a message of
    // max_sysex_size gets through whole. The cap keeps what is sent within
    // what SysexAssembler accepts on the other side.
    if (message.size() > max_sysex_size) {
        qWarning() << "Not sending a SysEx message of" << message.size() << "bytes";
        return false;
    }
    // snd_seq_ev_set_sysex() takes a non-const pointer, but ALSA only reads
    // the data: a copy keeps the const promise without a const_cast.
    QByteArray copy = message.toByteArray();

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, d->port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    // The ALSA macro clears a flag with '&= ~MASK' on an unsigned char,
    // which GCC reports as a sign conversion. The warning is about the
    // macro's code, not ours, so it is silenced for this line only.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    snd_seq_ev_set_sysex(&ev, static_cast<unsigned int>(copy.size()), copy.data());
#pragma GCC diagnostic pop

    const int ret = snd_seq_event_output_direct(d->handle(), &ev);
    if (ret < 0) {
        qWarning() << "Failed to send sysex:" << snd_strerror(ret);
        return false;
    }
    return true;
}

} // namespace lpd8
