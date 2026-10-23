#include "lpd8simulator.h"

#include <QSignalSpy>
#include <QtTest>

using lpd8::MidiPort;
using lpd8::MidiTransport;
using lpd8::Program;

namespace {

QByteArray bytes(std::initializer_list<int> values)
{
    QByteArray result;
    for (const int v : values) {
        result += static_cast<char>(v);
    }
    return result;
}

QByteArray getRequest(int slot)
{
    return bytes({0xf0, 0x47, 0x7f, 0x75, 0x63, 0x00, 0x01, slot, 0xf7});
}

// The program of the example in doc/SYSEX.md ("Get program"), field by
// field, and the example's response, as the document writes it. Neither
// goes through the codec: the simulator's answer is checked against the
// document, not against the code that will read it.
Program exampleProgram()
{
    Program p;
    p.channel = 0x06;
    const std::array<std::array<int, 4>, 8> pads{{{0x0c, 0x0d, 0x0e, 0},
                                                  {0x0f, 0x10, 0x11, 0},
                                                  {0x12, 0x13, 0x14, 1},
                                                  {0x15, 0x16, 0x17, 0},
                                                  {0x00, 0x01, 0x02, 0},
                                                  {0x03, 0x04, 0x05, 0},
                                                  {0x06, 0x07, 0x08, 0},
                                                  {0x09, 0x0a, 0x0b, 0}}};
    for (std::size_t i = 0; i < pads.size(); ++i) {
        p.pads.at(i) = {.note = static_cast<quint8>(pads.at(i)[0]),
                        .programChange = static_cast<quint8>(pads.at(i)[1]),
                        .controlChange = static_cast<quint8>(pads.at(i)[2]),
                        .toggle = pads.at(i)[3] == 1};
    }
    for (std::size_t i = 0; i < p.knobs.size(); ++i) {
        const auto first = static_cast<quint8>(0x18 + (3 * i));
        p.knobs.at(i) = {.controlChange = first,
                         .low = static_cast<quint8>(first + 1),
                         .high = static_cast<quint8>(first + 2)};
    }
    return p;
}

const QByteArray example_response =
        bytes({0xf0, 0x47, 0x7f, 0x75, 0x63, 0x00, 0x3a, 0x01, 0x06, 0x0c, 0x0d, 0x0e, 0x00, 0x0f,
               0x10, 0x11, 0x00, 0x12, 0x13, 0x14, 0x01, 0x15, 0x16, 0x17, 0x00, 0x00, 0x01, 0x02,
               0x00, 0x03, 0x04, 0x05, 0x00, 0x06, 0x07, 0x08, 0x00, 0x09, 0x0a, 0x0b, 0x00, 0x18,
               0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
               0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0xf7});

Program withChannel(quint8 channel)
{
    Program p = lpd8::profile().defaultProgram;
    p.channel = channel;
    return p;
}

constexpr int timeout_ms = 1000;

} // namespace

class TestLpd8Simulator : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void test_answerIsTheDocumentedExample();
    void test_answerComesOnALaterTurn();
    void test_sendThenGetRoundTripsEachSlot();
    void test_twoGetsAreAnsweredInOrder();
    void test_silentAndWrongSlotModes();
    void test_unrecognizedMessages_data();
    void test_unrecognizedMessages();
    void test_invalidValueNeverReachesTheDevice();
    void test_onlyHearsAndAnswersWhenConnected();
    void test_destroyedWithAPendingAnswer();
    void test_destroyingUnplugs();

private:
    std::unique_ptr<FakeTransport> transport;
    std::unique_ptr<Lpd8Simulator> device;
    std::unique_ptr<QSignalSpy> received;
};

void TestLpd8Simulator::init()
{
    transport = std::make_unique<FakeTransport>();
    device = std::make_unique<Lpd8Simulator>(*transport);
    QVERIFY(transport->connectTo(device->portId()));
    received = std::make_unique<QSignalSpy>(transport.get(), &MidiTransport::sysexReceived);
}

void TestLpd8Simulator::cleanup()
{
    received.reset();
    device.reset();
    transport.reset();
}

void TestLpd8Simulator::test_answerIsTheDocumentedExample()
{
    QCOMPARE(example_response.size(), 66);
    device->setProgram(1, exampleProgram());
    QVERIFY(transport->sendSysex(getRequest(1)));
    QVERIFY(received->wait(timeout_ms));
    QCOMPARE(received->at(0).at(0).toByteArray(), example_response);
    QCOMPARE(received->at(0).at(1).toString(), device->portId());
}

void TestLpd8Simulator::test_answerComesOnALaterTurn()
{
    // L1: never inside sendSysex(), as no device could.
    QVERIFY(transport->sendSysex(getRequest(2)));
    QCOMPARE(received->count(), 0);
    QCOMPARE(device->pendingAnswers(), 1);
    QVERIFY(received->wait(timeout_ms));
    QCOMPARE(received->count(), 1);
}

void TestLpd8Simulator::test_sendThenGetRoundTripsEachSlot()
{
    for (int slot = 1; slot <= 4; ++slot) {
        const Program program = withChannel(static_cast<quint8>(slot + 10));
        QVERIFY(transport->sendSysex(lpd8::encodeSendProgram(program, slot).value()));
        QCOMPARE(device->program(slot), program);
    }
    QCOMPARE(received->count(), 0); // a send has no answer

    for (int slot = 1; slot <= 4; ++slot) {
        received->clear();
        QVERIFY(transport->sendSysex(getRequest(slot)));
        QVERIFY(received->wait(timeout_ms));
        const auto decoded = lpd8::decodeProgram(received->at(0).at(0).toByteArray());
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->slot, slot);
        QCOMPARE(decoded->program, withChannel(static_cast<quint8>(slot + 10)));
    }
    QCOMPARE(device->requests(), 8);
    QCOMPARE(device->unrecognized(), 0);
}

void TestLpd8Simulator::test_twoGetsAreAnsweredInOrder()
{
    // L3: back to back, answered in order, one per turn.
    device->setProgram(1, withChannel(1));
    device->setProgram(2, withChannel(2));
    QVERIFY(transport->sendSysex(getRequest(1)));
    QVERIFY(transport->sendSysex(getRequest(2)));
    QVERIFY(QTest::qWaitFor([this] { return received->count() == 2; }, timeout_ms));
    QCOMPARE(lpd8::decodeProgram(received->at(0).at(0).toByteArray())->slot, 1);
    QCOMPARE(lpd8::decodeProgram(received->at(1).at(0).toByteArray())->slot, 2);
}

void TestLpd8Simulator::test_silentAndWrongSlotModes()
{
    device->setMode(Lpd8Simulator::Mode::Silent);
    QVERIFY(transport->sendSysex(getRequest(1)));
    QVERIFY(!received->wait(100));
    QCOMPARE(device->requests(), 1);

    device->setMode(Lpd8Simulator::Mode::WrongSlot);
    QVERIFY(transport->sendSysex(getRequest(4)));
    QVERIFY(received->wait(timeout_ms));
    QCOMPARE(lpd8::decodeProgram(received->at(0).at(0).toByteArray())->slot, 1);
}

void TestLpd8Simulator::test_unrecognizedMessages_data()
{
    // L2: what the device's behaviour is unknown for, or the application
    // doesn't use. Each is complete SysEx, so it reaches the device.
    QTest::addColumn<QByteArray>("message");

    const QByteArray send = lpd8::encodeSendProgram(lpd8::profile().defaultProgram, 1).value();
    QTest::newRow("Get slot 0") << getRequest(0);
    QTest::newRow("Get slot 5") << getRequest(5);
    QByteArray shortSend = send;
    shortSend.remove(10, 1);
    QTest::newRow("Send of the wrong size") << shortSend;
    QByteArray sendSlot5 = send;
    sendSlot5[7] = '\x05';
    QTest::newRow("Send to slot 5") << sendSlot5;
    QByteArray badChannel = send;
    badChannel[8] = '\x10'; // channel 17
    QTest::newRow("Send with a channel out of range") << badChannel;
    QTest::newRow("Get active program") << bytes({0xf0, 0x47, 0x7f, 0x75, 0x64, 0x00, 0x00, 0xf7});
    QTest::newRow("Another manufacturer") << bytes({0xf0, 0x41, 0x10, 0xf7});
}

void TestLpd8Simulator::test_unrecognizedMessages()
{
    QFETCH(QByteArray, message); // NOLINT(misc-const-correctness): QFETCH declares it

    QVERIFY(transport->sendSysex(message));
    QCOMPARE(device->unrecognized(), 1);
    QCOMPARE(device->requests(), 0);
    QVERIFY(!received->wait(100));
    for (int slot = 1; slot <= 4; ++slot) {
        QCOMPARE(device->program(slot), lpd8::profile().defaultProgram);
    }
}

void TestLpd8Simulator::test_invalidValueNeverReachesTheDevice()
{
    // A value above 7F is not SysEx: sendSysex() refuses it before any
    // device hears it.
    QByteArray send = lpd8::encodeSendProgram(lpd8::profile().defaultProgram, 1).value();
    send[9] = '\x80';
    QVERIFY(!transport->sendSysex(send));
    QCOMPARE(device->unrecognized(), 0);
    QCOMPARE(device->requests(), 0);
}

void TestLpd8Simulator::test_onlyHearsAndAnswersWhenConnected()
{
    // The application receives from the device, but doesn't send to it:
    // the device hears nothing (sendSysex() is refused: nobody listens).
    transport->setConnection(device->portId(), MidiPort::From);
    QVERIFY(!transport->sendSysex(getRequest(1)));
    QCOMPARE(device->requests(), 0);

    // It sends to it, but doesn't receive from it: the answer is lost.
    transport->setConnection(device->portId(), MidiPort::To);
    QVERIFY(transport->sendSysex(getRequest(1)));
    QCOMPARE(device->requests(), 1);
    QVERIFY(!received->wait(100));
}

void TestLpd8Simulator::test_destroyedWithAPendingAnswer()
{
    QVERIFY(transport->sendSysex(getRequest(1)));
    device.reset(); // unplugged before it answers
    QVERIFY(!received->wait(100));
}

void TestLpd8Simulator::test_destroyingUnplugs()
{
    const QString id = device->portId();
    const QSignalSpy portsChanged(transport.get(), &MidiTransport::portsChanged);
    const QSignalSpy connectionChanged(transport.get(), &MidiTransport::connectionChanged);
    device.reset();
    QVERIFY(std::ranges::none_of(transport->ports(),
                                 [&id](const MidiPort &port) { return port.id == id; }));
    QCOMPARE(portsChanged.count(), 1);
    QCOMPARE(connectionChanged.count(), 1); // it was connected
    QVERIFY(!transport->isConnected());

    // The fake is free for another device.
    device = std::make_unique<Lpd8Simulator>(*transport, QStringLiteral("24:0"));
    QVERIFY(transport->connectTo(QStringLiteral("24:0")));
}

QTEST_GUILESS_MAIN(TestLpd8Simulator)

#include "lpd8simulator_test.moc"
