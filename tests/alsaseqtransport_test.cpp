#include "alsaseqtransport.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>
#include <stdexcept>

using lpd8::AlsaSeqTransport;
using lpd8::MidiPort;
using lpd8::MidiTransport;

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

QTEST_GUILESS_MAIN(TestAlsaSeqTransport)

#include "alsaseqtransport_test.moc"
