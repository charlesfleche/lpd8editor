#include "lpd8device.h"
#include "lpd8simulator.h"

#include <QSignalSpy>
#include <QtTest>

using lpd8::Lpd8Device;
using lpd8::MidiPort;
using lpd8::Program;

namespace {

Program withChannel(quint8 channel)
{
    Program p = lpd8::profile().defaultProgram;
    p.channel = channel;
    return p;
}

// What the device sends back for 'slot': a dump, as the simulator builds it.
QByteArray dumpOf(const Program &program, int slot)
{
    QByteArray dump = lpd8::encodeStoredProgram(program);
    dump[6] = '\x3a';
    dump[7] = static_cast<char>(slot);
    return dump;
}

constexpr std::chrono::milliseconds short_timeout{100};
constexpr int wait_ms = 1000;

} // namespace

class TestLpd8Device : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void test_getIsAnswered();
    void test_oneRequestInFlight();
    void test_sendWaitsForTheGetBeforeIt();
    void test_timeout();
    void test_wrongSlotIsIgnoredAndTimesOut();
    void test_otherClientsAndUnsolicitedDumpsAreIgnored();
    void test_replugUnderANewId();
    void test_unpluggedDuringARequest();
    void test_notConnected();
    void test_answerDuringTheSend();
    void test_connectionsMadeByAnotherTool();
    void test_aMonitorAddedDuringARequest();

private:
    std::unique_ptr<FakeTransport> transport;
    std::unique_ptr<Lpd8Simulator> simulator;
    std::unique_ptr<Lpd8Device> device;
    std::unique_ptr<QSignalSpy> received;
    std::unique_ptr<QSignalSpy> failed;
    std::unique_ptr<QSignalSpy> busy;
};

void TestLpd8Device::init()
{
    transport = std::make_unique<FakeTransport>();
    simulator = std::make_unique<Lpd8Simulator>(*transport);
    QVERIFY(transport->connectTo(simulator->portId()));
    device = std::make_unique<Lpd8Device>(*transport);
    device->setReplyTimeout(short_timeout);
    received = std::make_unique<QSignalSpy>(device.get(), &Lpd8Device::programReceived);
    failed = std::make_unique<QSignalSpy>(device.get(), &Lpd8Device::requestFailed);
    busy = std::make_unique<QSignalSpy>(device.get(), &Lpd8Device::busyChanged);
}

void TestLpd8Device::cleanup()
{
    busy.reset();
    failed.reset();
    received.reset();
    device.reset();
    simulator.reset();
    transport.reset();
}

void TestLpd8Device::test_getIsAnswered()
{
    simulator->setProgram(2, withChannel(5));
    device->requestProgram(2);
    QVERIFY(device->isBusy());
    QVERIFY(received->wait(wait_ms));
    QCOMPARE(received->count(), 1);
    QCOMPARE(received->at(0).at(0).toInt(), 2);
    QCOMPARE(received->at(0).at(1).value<Program>(), withChannel(5));
    QVERIFY(!device->isBusy());
    QCOMPARE(busy->count(), 2); // busy, then not
    QCOMPARE(failed->count(), 0);
}

void TestLpd8Device::test_oneRequestInFlight()
{
    for (int slot = 1; slot <= 4; ++slot) {
        simulator->setProgram(slot, withChannel(static_cast<quint8>(slot)));
    }
    device->requestProgram(1);
    device->requestProgram(2);
    device->requestProgram(3);
    QCOMPARE(transport->sent().size(), 1); // the others wait

    QVERIFY(QTest::qWaitFor([this] { return received->count() == 3; }, wait_ms));
    QCOMPARE(transport->sent().size(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(received->at(i).at(0).toInt(), i + 1);
        QCOMPARE(received->at(i).at(1).value<Program>(), withChannel(static_cast<quint8>(i + 1)));
    }
    QCOMPARE(busy->count(), 2); // busy once, through the three
}

void TestLpd8Device::test_sendWaitsForTheGetBeforeIt()
{
    const QSignalSpy done(device.get(), &Lpd8Device::sendDone);
    device->requestProgram(1);
    device->sendProgram(2, withChannel(7));
    QCOMPARE(transport->sent().size(), 1); // the send waits for the answer

    QVERIFY(QTest::qWaitFor([&done] { return done.count() == 1; }, wait_ms));
    QCOMPARE(received->count(), 1);
    QCOMPARE(done.at(0).at(0).toInt(), 2);
    QCOMPARE(simulator->program(2), withChannel(7));
    QVERIFY(!device->isBusy());
}

void TestLpd8Device::test_timeout()
{
    simulator->setMode(Lpd8Simulator::Mode::Silent);
    device->requestProgram(3);
    device->requestProgram(4);
    QVERIFY(failed->wait(wait_ms));
    QCOMPARE(failed->at(0).at(0).toInt(), 3);
    QVERIFY(failed->at(0).at(1).toString().contains(QStringLiteral("did not answer")));
    QCOMPARE(received->count(), 0);

    // The next request goes once the first has failed, and fails too.
    QVERIFY(QTest::qWaitFor([this] { return failed->count() == 2; }, wait_ms));
    QCOMPARE(transport->sent().size(), 2);
    QVERIFY(!device->isBusy());
}

void TestLpd8Device::test_wrongSlotIsIgnoredAndTimesOut()
{
    simulator->setMode(Lpd8Simulator::Mode::WrongSlot);
    device->requestProgram(1); // answered with slot 2
    QVERIFY(failed->wait(wait_ms));
    QCOMPARE(received->count(), 0);
    QCOMPARE(device->ignoredMessages(), 1);
}

void TestLpd8Device::test_otherClientsAndUnsolicitedDumpsAreIgnored()
{
    // The security finding: another local client sends a dump. It is not
    // the connected device's, so it is never taken as the answer.
    transport->addPort("other:0", "Another client");
    simulator->setProgram(1, withChannel(1));
    device->requestProgram(1);
    transport->receive(dumpOf(withChannel(15), 1), "other:0");
    QCOMPARE(received->count(), 0);
    QCOMPARE(device->ignoredMessages(), 1);

    // The device's own answer still is.
    QVERIFY(received->wait(wait_ms));
    QCOMPARE(received->at(0).at(1).value<Program>(), withChannel(1));

    // A dump from the device itself, with nothing asked: ignored too.
    transport->receive(dumpOf(withChannel(15), 1), simulator->portId());
    QCOMPARE(received->count(), 1);
    QCOMPARE(device->ignoredMessages(), 2);
}

void TestLpd8Device::test_replugUnderANewId()
{
    // M1: the device comes back as 24:0. 20:0 may now name something else:
    // what comes from it is not the device's.
    simulator.reset(); // unplugged
    simulator = std::make_unique<Lpd8Simulator>(*transport, QStringLiteral("24:0"));
    QVERIFY(transport->connectTo(QStringLiteral("24:0")));
    simulator->setProgram(1, withChannel(1));

    device->requestProgram(1);
    transport->receive(dumpOf(withChannel(15), 1), QStringLiteral("20:0"));
    QCOMPARE(device->ignoredMessages(), 1);
    QVERIFY(received->wait(wait_ms));
    QCOMPARE(received->at(0).at(1).value<Program>(), withChannel(1));
}

void TestLpd8Device::test_unpluggedDuringARequest()
{
    simulator->setMode(Lpd8Simulator::Mode::Silent);
    device->requestProgram(1);
    device->requestProgram(2);
    simulator.reset(); // unplugged, with a get in flight and one waiting

    // At once, not at the timeout, for both.
    QCOMPARE(failed->count(), 2);
    QVERIFY(failed->at(0).at(1).toString().contains(QStringLiteral("disconnected")));
    QVERIFY(!device->isBusy());
    QVERIFY(!failed->wait(2 * static_cast<int>(short_timeout.count())));
    QCOMPARE(failed->count(), 2); // and nothing more later
}

void TestLpd8Device::test_notConnected()
{
    transport->disconnectAll();
    device->requestProgram(1);
    QCOMPARE(failed->count(), 1);
    QVERIFY(failed->at(0).at(1).toString().contains(QStringLiteral("Not connected")));
    QCOMPARE(transport->sent().size(), 0);
    QVERIFY(!device->isBusy());
}

void TestLpd8Device::test_answerDuringTheSend()
{
    // L1: no reentrancy window. A real backend never answers inside the
    // send call; this test's own hook does, on purpose, to prove that the
    // request is in flight before its message goes. No simulator here: the
    // fake has one hook.
    simulator.reset();
    transport->addPort("30:0", "Reentrant LPD8");
    QVERIFY(transport->connectTo(QStringLiteral("30:0")));
    transport->setSendHook([this](const QByteArray &message) {
        const int slot = static_cast<unsigned char>(message.at(7));
        transport->receive(dumpOf(withChannel(static_cast<quint8>(slot)), slot),
                           QStringLiteral("30:0"));
    });

    device->requestProgram(1);
    device->requestProgram(2);
    // Both answered inside their sends, in order, with no timeout later.
    QCOMPARE(received->count(), 2);
    QCOMPARE(received->at(0).at(0).toInt(), 1);
    QCOMPARE(received->at(1).at(0).toInt(), 2);
    QVERIFY(!device->isBusy());
    QVERIFY(!failed->wait(2 * static_cast<int>(short_timeout.count())));
    transport->setSendHook({});
}

void TestLpd8Device::test_connectionsMadeByAnotherTool()
{
    // E1: patched with aconnect or qjackctl, the LPD8 both ways and a MIDI
    // monitor that receives what the application sends. No single
    // "connected port" (externally managed), and it works, as before.
    transport->disconnectAll();
    transport->addPort("40:0", "MIDI monitor");
    transport->setConnection(simulator->portId(), MidiPort::BothWays);
    transport->setConnection("40:0", MidiPort::To);
    QVERIFY(transport->externallyManaged());
    simulator->setProgram(1, withChannel(1));

    device->requestProgram(1);
    // A dump from the monitor (the application only sends to it) or from a
    // client not patched in at all is not an answer.
    transport->addPort("other:0", "Another client");
    transport->receive(dumpOf(withChannel(15), 1), "40:0");
    transport->receive(dumpOf(withChannel(15), 1), "other:0");
    QCOMPARE(device->ignoredMessages(), 2);
    QCOMPARE(received->count(), 0);

    // The LPD8's is.
    QVERIFY(received->wait(wait_ms));
    QCOMPARE(received->at(0).at(1).value<Program>(), withChannel(1));
    QCOMPARE(failed->count(), 0);
}

void TestLpd8Device::test_aMonitorAddedDuringARequest()
{
    // Another tool adds a monitor while a get is in flight: the request goes
    // on, since the device still sends to the application.
    simulator->setProgram(2, withChannel(2));
    device->requestProgram(2);
    transport->addPort("40:0", "MIDI monitor");
    transport->setConnection("40:0", MidiPort::To);
    QCOMPARE(failed->count(), 0);
    QVERIFY(received->wait(wait_ms));
    QCOMPARE(received->at(0).at(0).toInt(), 2);
}

QTEST_GUILESS_MAIN(TestLpd8Device)

#include "lpd8device_test.moc"
