#include "alsaseqtransport.h"
#include "deviceprofile.h"
#include "lpd8codec.h"
#include "lpd8device.h"
#include "sysexassembler.h"

#include <QSignalSpy>
#include <QtTest>

#include <alsa/asoundlib.h>

#include <memory>
#include <stdexcept>

using lpd8::AlsaSeqTransport;
using lpd8::MidiPort;
using lpd8::MidiTransport;
using lpd8::SysexAssembler;

namespace {

// A "get program 1" request, and a message of the largest size sent, both
// built with their size (they hold zero bytes).
const QByteArray small("\xf0\x47\x7f\x75\x63\x00\x01\x01\xf7", 9);

QByteArray largest()
{
    QByteArray message(AlsaSeqTransport::max_sysex_size, '\x00');
    for (qsizetype i = 1; i < message.size() - 1; ++i) {
        message[i] = static_cast<char>(i % 128);
    }
    message.front() = '\xf0';
    message.back() = '\xf7';
    return message;
}

bool lists(const MidiTransport &transport, const QString &id)
{
    const QList<MidiPort> ports = transport.ports();
    return std::ranges::find(ports, id, &MidiPort::id) != ports.end();
}

// The ALSA address of a port id ("client:port").
snd_seq_addr_t addrOf(const QString &id)
{
    const QStringList parts = id.split(u':');
    Q_ASSERT(parts.size() == 2);
    return {.client = static_cast<unsigned char>(parts.at(0).toUInt()),
            .port = static_cast<unsigned char>(parts.at(1).toUInt())};
}

// Another ALSA client, standing for any local process that can open the
// sequencer: it sends events of its choice straight to a port, without a
// subscription, as the kernel allows.
class RawClient
{
public:
    RawClient()
    {
        snd_seq_t *seq = nullptr;
        if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
            return;
        }
        m_seq.reset(seq);
        m_port = snd_seq_create_simple_port(seq,
                                            "raw",
                                            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE,
                                            SND_SEQ_PORT_TYPE_APPLICATION);
    }

    bool isOpen() const { return m_seq && m_port >= 0; }
    QString id() const
    {
        return QStringLiteral("%1:%2").arg(QString::number(snd_seq_client_id(m_seq.get())),
                                           QString::number(m_port));
    }

    // Another port of this client, or -1. Any client can create ports, up
    // to 254 (SNDRV_SEQ_MAX_PORTS), without privileges.
    int addPort() const
    {
        return snd_seq_create_simple_port(
                m_seq.get(), "raw", SND_SEQ_PORT_CAP_READ, SND_SEQ_PORT_TYPE_APPLICATION);
    }

    // Sends 'ev' to 'dest', at once, from 'sourcePort': by default the
    // client's first port. The kernel only accepts a port of the sending
    // client as the source (and sets the client itself).
    bool sendTo(const QString &dest, snd_seq_event_t ev, int sourcePort = -1) const
    {
        const snd_seq_addr_t addr = addrOf(dest);
        snd_seq_ev_set_source(&ev,
                              static_cast<unsigned char>(sourcePort < 0 ? m_port : sourcePort));
        snd_seq_ev_set_dest(&ev, addr.client, addr.port);
        snd_seq_ev_set_direct(&ev);
        return snd_seq_event_output_direct(m_seq.get(), &ev) >= 0;
    }

    // A SysEx event whose bytes travel with it (what alsa-lib sends).
    bool sendSysex(const QString &dest, QByteArray &bytes, int sourcePort = -1) const
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);
        // Not snd_seq_ev_set_sysex(): GCC reports a sign conversion in it.
        ev.type = SND_SEQ_EVENT_SYSEX;
        ev.flags = SND_SEQ_EVENT_LENGTH_VARIABLE;
        ev.data.ext.len = static_cast<unsigned int>(bytes.size());
        ev.data.ext.ptr = bytes.data();
        return sendTo(dest, ev, sourcePort);
    }

private:
    struct Closer
    {
        void operator()(snd_seq_t *handle) const { snd_seq_close(handle); }
    };
    std::unique_ptr<snd_seq_t, Closer> m_seq;
    int m_port{-1};
};

// Waits use QTest::qWaitFor() rather than the QTRY macros, which store a
// std::chrono count in an int that GCC's -Wconversion reports here
// (qtestcase.h, Qt 6.8).
constexpr int timeout_ms = 2000;

} // namespace

// The only test that needs a real ALSA sequencer (/dev/snd/seq). It skips,
// saying why, without one: CI machines usually have none (doc/IMPROVEMENTS.md,
// item 4.8b, M4). Two transports talk to each other through the kernel.
class TestAlsaSeqTransport : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_otherClientsAreListed();
    void test_loopback();
    void test_largestMessageArrivesWhole();
    void test_hotplugAfterConnecting();
    void test_dropsEventsThatPointIntoAnotherProcess();
    void test_floodDoesNotBlockTheEventLoop();
    void test_ignoresForgedAnnouncements();
    void test_unfinishedMessagesAreBounded();
    void test_deviceIgnoresAForgedReply();

private:
    std::unique_ptr<AlsaSeqTransport> a;
    std::unique_ptr<AlsaSeqTransport> b;
    QByteArray skipReason;
};

void TestAlsaSeqTransport::initTestCase()
{
    try {
        const AlsaSeqTransport probe;
    } catch (const std::exception &e) {
        skipReason = QByteArray("No ALSA sequencer available (no /dev/snd/seq?), the real "
                                "MIDI backend is NOT tested: ")
                + e.what();
    }
}

void TestAlsaSeqTransport::init()
{
    if (!skipReason.isEmpty()) {
        QSKIP(skipReason.constData());
    }
    a = std::make_unique<AlsaSeqTransport>();
    b = std::make_unique<AlsaSeqTransport>();
}

void TestAlsaSeqTransport::cleanup()
{
    b.reset();
    a.reset();
}

void TestAlsaSeqTransport::test_otherClientsAreListed()
{
    // B was opened after A: A hears of it through the announcements, and B
    // finds A already there. Neither lists itself.
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, b->ownPortId()); }, timeout_ms));
    QVERIFY(lists(*b, a->ownPortId()));
    QVERIFY(!lists(*a, a->ownPortId()));
    QVERIFY(!a->isConnected());
}

void TestAlsaSeqTransport::test_loopback()
{
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, b->ownPortId()); }, timeout_ms));
    QSignalSpy bChanged(b.get(), &MidiTransport::connectionChanged);
    QVERIFY(a->connectTo(b->ownPortId()));
    QCOMPARE(a->connectedPort(), b->ownPortId());
    QVERIFY(!a->externallyManaged());
    // B sees the connection too, made by another client: both ways, so it
    // counts as connected (the old semantics, see MidiConnectionsModel).
    QVERIFY(QTest::qWaitFor([&] { return bChanged.count() > 0; }, timeout_ms));
    QCOMPARE(b->connectedPort(), a->ownPortId());

    // A to B, and B to A, each with the sender's id as the source.
    QSignalSpy bReceived(b.get(), &MidiTransport::sysexReceived);
    QVERIFY(a->sendSysex(small));
    QVERIFY(bReceived.wait(timeout_ms));
    QCOMPARE(bReceived.at(0).at(0).toByteArray(), small);
    QCOMPARE(bReceived.at(0).at(1).toString(), a->ownPortId());

    QSignalSpy aReceived(a.get(), &MidiTransport::sysexReceived);
    QVERIFY(b->sendSysex(small));
    QVERIFY(aReceived.wait(timeout_ms));
    QCOMPARE(aReceived.at(0).at(1).toString(), b->ownPortId());

    // Disconnecting is seen on both sides.
    bChanged.clear();
    a->disconnectAll();
    QVERIFY(!a->isConnected());
    // isConnected() asks ALSA, so it is right at once; the signal comes
    // with the announcement, through the event loop.
    QVERIFY(!b->isConnected());
    QVERIFY(QTest::qWaitFor([&] { return bChanged.count() > 0; }, timeout_ms));
    QVERIFY(!a->sendSysex(small)); // nobody listens any more
}

void TestAlsaSeqTransport::test_largestMessageArrivesWhole()
{
    // Far above an LPD8 message (66 bytes): the largest one sendSysex()
    // sends gets through as one message, reassembled if ALSA cuts it.
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, b->ownPortId()); }, timeout_ms));
    QVERIFY(a->connectTo(b->ownPortId()));
    QSignalSpy received(b.get(), &MidiTransport::sysexReceived);
    const QByteArray message = largest();
    QVERIFY(a->sendSysex(message));
    QVERIFY(received.wait(timeout_ms));
    QCOMPARE(received.count(), 1);
    QCOMPARE(received.at(0).at(0).toByteArray(), message);

    // One byte more is refused, not sent.
    QByteArray tooLong = message;
    tooLong.insert(1, '\x01');
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Not sending a SysEx message"));
    QVERIFY(!a->sendSysex(tooLong));
}

void TestAlsaSeqTransport::test_hotplugAfterConnecting()
{
    // Connecting must not drop the announcement subscription: a client that
    // comes later is still seen, and one that goes is removed.
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, b->ownPortId()); }, timeout_ms));
    QVERIFY(a->connectTo(b->ownPortId()));
    a->disconnectAll();

    auto c = std::make_unique<AlsaSeqTransport>();
    const QString cId = c->ownPortId();
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, cId); }, timeout_ms));

    const QSignalSpy portsChanged(a.get(), &MidiTransport::portsChanged);
    c.reset();
    QVERIFY(QTest::qWaitFor([&] { return !lists(*a, cId); }, timeout_ms));
    QVERIFY(portsChanged.count() > 0);
}

void TestAlsaSeqTransport::test_dropsEventsThatPointIntoAnotherProcess()
{
    // A SysEx event with the 'user variable length' (VARUSR) length type
    // carries a pointer into the *sender's* memory, and the kernel delivers
    // it unchanged: the sender chooses the address a reader of ext.ptr reads
    // from. Reading it crashed the application (an address mapped nowhere)
    // or read its memory where the sender chose. It must be dropped, and
    // the events after it still read.
    const RawClient sender;
    QVERIFY(sender.isOpen());
    QSignalSpy received(a.get(), &MidiTransport::sysexReceived);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.type = SND_SEQ_EVENT_SYSEX;
    ev.flags = SND_SEQ_EVENT_LENGTH_VARUSR;
    ev.data.ext.len = 66;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
    ev.data.ext.ptr = reinterpret_cast<void *>(0x4141414141414141ULL);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Ignoring a MIDI event with a foreign"));
    QVERIFY(sender.sendTo(a->ownPortId(), ev));

    QByteArray after = small;
    QVERIFY(sender.sendSysex(a->ownPortId(), after));
    QVERIFY(received.wait(timeout_ms));
    QCOMPARE(received.count(), 1);
    QCOMPARE(received.at(0).at(0).toByteArray(), small);
}

void TestAlsaSeqTransport::test_floodDoesNotBlockTheEventLoop()
{
    // A client that answers each message it sees arrive with a new one:
    // there is always one more event to read. If the transport read until
    // nothing is left, it would never return to the event loop, and the
    // window would freeze. The timer stands for the rest of the
    // application, which must still get its turn.
    const RawClient sender;
    QVERIFY(sender.isOpen());
    const QString dest = a->ownPortId();

    constexpr int give_up = 20000; // ends the test if the loop never yields
    bool timerFired = false;
    int received = 0;
    connect(a.get(),
            &MidiTransport::sysexReceived,
            this,
            [this, &sender, &dest, &timerFired, &received] {
                if (timerFired || ++received == give_up) {
                    return;
                }
                if (received == 1) {
                    // Armed while the transport reads: it can only fire once
                    // the transport gives control back.
                    QTimer::singleShot(0, this, [&timerFired] { timerFired = true; });
                }
                QByteArray next = small;
                sender.sendSysex(dest, next);
            });
    QByteArray first = small;
    QVERIFY(sender.sendSysex(dest, first));

    QVERIFY(QTest::qWaitFor([&timerFired] { return timerFired; }, timeout_ms));
    QVERIFY2(received < give_up, "The transport never returned to the event loop");
    disconnect(a.get(), &MidiTransport::sysexReceived, this, nullptr);
}

void TestAlsaSeqTransport::test_ignoresForgedAnnouncements()
{
    // The transport follows hotplug through the announcements of the
    // system client: 'port exited', 'port subscribed'... Any other client
    // can send events of the same types straight to our port. A forged
    // 'port exited' for B removed B from the list while it still exists:
    // the user could no longer pick the device, and Lpd8Device, which only
    // takes replies from listed ports, failed its requests.
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, b->ownPortId()); }, timeout_ms));
    const RawClient forger;
    QVERIFY(forger.isOpen());
    // The forger's own port is real: it is announced, and listed.
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, forger.id()); }, timeout_ms));
    const QSignalSpy portsChanged(a.get(), &MidiTransport::portsChanged);
    const QSignalSpy connectionChanged(a.get(), &MidiTransport::connectionChanged);
    QSignalSpy received(a.get(), &MidiTransport::sysexReceived);

    const snd_seq_addr_t target = addrOf(b->ownPortId());
    for (const auto type : {SND_SEQ_EVENT_PORT_EXIT, SND_SEQ_EVENT_PORT_CHANGE}) {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);
        ev.type = type;
        ev.data.addr = target;
        QVERIFY(forger.sendTo(a->ownPortId(), ev));
    }
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.type = SND_SEQ_EVENT_PORT_SUBSCRIBED;
    ev.data.connect.sender = target;
    ev.data.connect.dest = addrOf(a->ownPortId());
    QVERIFY(forger.sendTo(a->ownPortId(), ev));

    // Events from one client arrive in order: once this message is in, the
    // forged events were handled.
    QByteArray after = small;
    QVERIFY(forger.sendSysex(a->ownPortId(), after));
    QVERIFY(received.wait(timeout_ms));

    QVERIFY(lists(*a, b->ownPortId()));
    QCOMPARE(portsChanged.count(), 0);
    QCOMPARE(connectionChanged.count(), 0);
}

void TestAlsaSeqTransport::test_unfinishedMessagesAreBounded()
{
    // The bytes of each source are reassembled apart (SysexAssembler), and
    // each assembler holds up to SysexAssembler::default_max_size bytes of
    // an unfinished message, kept until the message ends. A source is a
    // port, and any client can create ports: one client with 254 ports,
    // each starting a message and never ending it, made the transport keep
    // 254 assemblers, about 1 MB, and the 64 clients ALSA allows about
    // 64 MB, for as long as the application runs.
    const RawClient flooder;
    QVERIFY(flooder.isOpen());
    QSignalSpy received(a.get(), &MidiTransport::sysexReceived);

    QByteArray unfinished(SysexAssembler::default_max_size - 2, '\x01');
    unfinished.front() = '\xf0';
    constexpr int ports = 64; // well over the cap, far below ALSA's limit
    for (int i = 0; i < ports; ++i) {
        const int port = flooder.addPort();
        QVERIFY(port >= 0);
        QVERIFY(flooder.sendSysex(a->ownPortId(), unfinished, port));
        // Paced, as an attacker would: the receiver's input pool holds only
        // a few such messages, and the kernel drops all of them when it
        // overflows. A complete message from the first port follows each
        // one: events from one client arrive in order, so once it is in,
        // the unfinished one was handled.
        QByteArray sync = small;
        QVERIFY(flooder.sendSysex(a->ownPortId(), sync));
        QVERIFY(QTest::qWaitFor([&] { return received.count() == i + 1; }, timeout_ms));
    }

    const qsizetype bound =
            AlsaSeqTransport::max_unfinished_sources * SysexAssembler::default_max_size;
    QVERIFY2(a->bufferedSysexBytes() <= bound,
             qPrintable(QString::number(a->bufferedSysexBytes())));

    // A message cut in two pieces still arrives whole: reassembly works.
    QByteArray head = small.first(4);
    QByteArray tail = small.sliced(4);
    QVERIFY(flooder.sendSysex(a->ownPortId(), head));
    QVERIFY(flooder.sendSysex(a->ownPortId(), tail));
    QVERIFY(received.wait(timeout_ms));
    QCOMPARE(received.at(ports).at(0).toByteArray(), small);
}

void TestAlsaSeqTransport::test_deviceIgnoresAForgedReply()
{
    // Finding 2 of the security review, end to end: while lpd8editor (A)
    // waits for the LPD8 (B) to answer 'get program 1', another local
    // client sends a dump for slot 1 straight to A. Lpd8Device takes a
    // reply only from a port A receives from, and the transport names the
    // source with the client id the kernel wrote, which the forger can't
    // choose: the forged dump is ignored, and B's answer is taken.
    // lpd8device_test checks the rule with a fake transport; this checks
    // that the ALSA transport gives it a source it can trust.
    QVERIFY(QTest::qWaitFor([&] { return lists(*a, b->ownPortId()); }, timeout_ms));
    QVERIFY(a->connectTo(b->ownPortId()));
    lpd8::Lpd8Device device(*a);
    const QSignalSpy got(&device, &lpd8::Lpd8Device::programReceived);
    QSignalSpy asked(b.get(), &MidiTransport::sysexReceived);

    const auto dump = [](quint8 channel) {
        lpd8::Program program = lpd8::profile().defaultProgram;
        program.channel = channel;
        QByteArray message = lpd8::encodeStoredProgram(program);
        message[6] = '\x3a'; // 63 00 3A: a dump, as the device sends
        return message;
    };

    device.requestProgram(1);
    QVERIFY(asked.wait(timeout_ms)); // B got the request

    const RawClient forger;
    QVERIFY(forger.isOpen());
    QByteArray forged = dump(15);
    QVERIFY(forger.sendSysex(a->ownPortId(), forged));
    QVERIFY(QTest::qWaitFor([&] { return device.ignoredMessages() == 1; }, timeout_ms));
    QCOMPARE(got.count(), 0);

    QVERIFY(b->sendSysex(dump(1)));
    QVERIFY(QTest::qWaitFor([&] { return got.count() == 1; }, timeout_ms));
    QCOMPARE(got.at(0).at(1).value<lpd8::Program>().channel, 1);
}

QTEST_GUILESS_MAIN(TestAlsaSeqTransport)

#include "alsaseqtransport_test.moc"
