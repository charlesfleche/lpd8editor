#include "faketransport.h"

#include <QSignalSpy>
#include <QtTest>

using lpd8::MidiPort;
using lpd8::MidiTransport;

namespace {

// A "get program 1" request. Built with its size: it holds a zero byte,
// where a C string would stop.
const QByteArray message("\xf0\x47\x7f\x75\x63\x00\x01\x01\xf7", 9);

} // namespace

// The rules every MidiTransport shares (connectedPort(), externallyManaged(),
// sendSysex()), through the fake the other tests use, and that fake's
// behaviour on replug, which later tests rely on.
class TestMidiTransport : public QObject
{
    Q_OBJECT

private slots:
    void test_connectedPortAndExternallyManaged_data();
    void test_connectedPortAndExternallyManaged();
    void test_connectToConnectsOnePortBothWays();
    void test_sendSysexNeedsAListenerAndAMessage();
    void test_isCompleteSysex_data();
    void test_isCompleteSysex();
    void test_replugGivesANewIdAndNoConnection();
};

void TestMidiTransport::test_connectedPortAndExternallyManaged_data()
{
    using Connections = QList<MidiPort::Directions>; // ports "a" and "b"
    QTest::addColumn<Connections>("connections");
    QTest::addColumn<QString>("connectedPort"); // empty: none
    QTest::addColumn<bool>("connected");
    QTest::addColumn<bool>("external");

    const MidiPort::Directions none = MidiPort::Disconnected;
    const MidiPort::Directions both = MidiPort::BothWays;
    QTest::newRow("Nothing") << Connections{none, none} << QString() << false << false;
    QTest::newRow("One port both ways") << Connections{none, both} << "b" << true << false;
    QTest::newRow("One way only") << Connections{MidiPort::Directions(MidiPort::From), none}
                                  << QString() << true << true;
    QTest::newRow("Two ports") << Connections{both, both} << QString() << true << true;
    QTest::newRow("Both ways and another one way")
            << Connections{both, MidiPort::Directions(MidiPort::To)} << QString() << true << true;
}

void TestMidiTransport::test_connectedPortAndExternallyManaged()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QList<MidiPort::Directions>, connections);
    QFETCH(QString, connectedPort);
    QFETCH(bool, connected);
    QFETCH(bool, external);
    // NOLINTEND(misc-const-correctness)

    FakeTransport transport;
    transport.addPort("a", "Device A");
    transport.addPort("b", "Device B");
    transport.setConnection("a", connections.at(0));
    transport.setConnection("b", connections.at(1));

    QCOMPARE(transport.connectedPort().value_or(QString()), connectedPort);
    QCOMPARE(transport.isConnected(), connected);
    QCOMPARE(transport.externallyManaged(), external);
}

void TestMidiTransport::test_connectToConnectsOnePortBothWays()
{
    FakeTransport transport;
    transport.addPort("a", "Device A");
    transport.addPort("b", "Device B");
    const QSignalSpy changed(&transport, &MidiTransport::connectionChanged);

    QVERIFY(transport.connectTo("a"));
    QCOMPARE(transport.connectedPort(), QString("a"));
    QVERIFY(transport.connectTo("b")); // the previous connection goes
    QCOMPARE(transport.connectedPort(), QString("b"));
    QCOMPARE(transport.ports().at(0).connection, MidiPort::Directions(MidiPort::Disconnected));
    QVERIFY(!transport.connectTo("no such port"));
    QCOMPARE(transport.connectedPort(), QString("b"));
    QCOMPARE(changed.count(), 2);

    transport.disconnectAll();
    QVERIFY(!transport.isConnected());
}

void TestMidiTransport::test_sendSysexNeedsAListenerAndAMessage()
{
    FakeTransport transport;
    transport.addPort("a", "Device A");

    // Nothing receives from the application's port: false, nothing sent.
    QVERIFY(!transport.sendSysex(message));
    // Receiving from it only is not enough either.
    transport.setConnection("a", MidiPort::From);
    QVERIFY(!transport.sendSysex(message));
    QVERIFY(transport.sent().isEmpty());

    // A listener, even one another tool connected: sent.
    transport.setConnection("a", MidiPort::To);
    QVERIFY(transport.sendSysex(message));
    QCOMPARE(transport.sent(), QList<QByteArray>{message});

    // Not a complete message: refused.
    QVERIFY(!transport.sendSysex(message.first(5)));
    QCOMPARE(transport.sent().size(), 1);
}

void TestMidiTransport::test_isCompleteSysex_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<bool>("complete");

    QTest::newRow("Program request") << message << true;
    QTest::newRow("Empty SysEx") << QByteArray("\xf0\xf7") << true;
    QTest::newRow("Nothing") << QByteArray() << false;
    QTest::newRow("No end") << message.first(message.size() - 1) << false;
    QTest::newRow("No start") << message.sliced(1) << false;
    QTest::newRow("Status byte inside") << QByteArray("\xf0\x47\x90\xf7") << false;
    QTest::newRow("Two messages") << message + message << false;
}

void TestMidiTransport::test_isCompleteSysex()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QByteArray, bytes);
    QFETCH(bool, complete);
    // NOLINTEND(misc-const-correctness)

    QCOMPARE(MidiTransport::isCompleteSysex(bytes), complete);
}

void TestMidiTransport::test_replugGivesANewIdAndNoConnection()
{
    // M1: ALSA gives a replugged device a new client number. The old id is
    // gone with its connection, and the new one is a new, unconnected port.
    FakeTransport transport;
    transport.addPort("20:0", "LPD8");
    QVERIFY(transport.connectTo("20:0"));
    QSignalSpy received(&transport, &MidiTransport::sysexReceived);
    transport.receive(message, "20:0");
    QCOMPARE(received.takeFirst().at(1).toString(), QString("20:0"));

    const QSignalSpy portsChanged(&transport, &MidiTransport::portsChanged);
    const QSignalSpy connectionChanged(&transport, &MidiTransport::connectionChanged);
    transport.removePort("20:0"); // unplugged
    QCOMPARE(portsChanged.count(), 1);
    QCOMPARE(connectionChanged.count(), 1);
    QVERIFY(!transport.isConnected());
    QVERIFY(!transport.sendSysex(message));

    transport.addPort("24:0", "LPD8"); // plugged back
    QVERIFY(!transport.isConnected()); // not connected again by itself
    QVERIFY(transport.connectTo("24:0"));
    QCOMPARE(transport.connectedPort(), QString("24:0"));
    transport.receive(message, "24:0");
    QCOMPARE(received.takeFirst().at(1).toString(), QString("24:0"));
    QVERIFY(!transport.connectTo("20:0")); // the old id names nothing
}

QTEST_GUILESS_MAIN(TestMidiTransport)

#include "miditransport_test.moc"
