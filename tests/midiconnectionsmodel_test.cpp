#include "faketransport.h"
#include "midiconnectionsmodel.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QtTest>

using lpd8::MidiPort;

class TestMidiConnectionsModel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void test_rows();
    void test_hotplugInsertsAndRemovesRows();
    void test_connectPort();
    void test_connectPortSignalsValidRange();
    void test_otherToolsConnections();

private:
    std::unique_ptr<FakeTransport> transport;
    std::unique_ptr<MidiConnectionsModel> model;
    std::unique_ptr<QAbstractItemModelTester> tester;
};

void TestMidiConnectionsModel::init()
{
    transport = std::make_unique<FakeTransport>();
    transport->addPort("20:0", "LPD8");
    transport->addPort("28:0", "Synth");
    model = std::make_unique<MidiConnectionsModel>(*transport);
    // Checks the QAbstractItemModel rules on every change of the model, and
    // fails the test on the first violation.
    tester = std::make_unique<QAbstractItemModelTester>(
            model.get(), QAbstractItemModelTester::FailureReportingMode::Fatal);
}

void TestMidiConnectionsModel::cleanup()
{
    tester.reset();
    model.reset();
    transport.reset();
}

void TestMidiConnectionsModel::test_rows()
{
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->index(0).data().toString(), QStringLiteral("Disconnect"));
    QCOMPARE(model->index(1).data().toString(), QStringLiteral("LPD8"));
    QCOMPARE(model->index(2).data().toString(), QStringLiteral("Synth"));
    QCOMPARE(model->connectedPort(), model->index(0)); // nothing connected
    QVERIFY(!model->connected());
}

void TestMidiConnectionsModel::test_hotplugInsertsAndRemovesRows()
{
    // Rows come and go, without a reset: the combo box keeps its item.
    const QSignalSpy reset(model.get(), &QAbstractItemModel::modelReset);
    const QSignalSpy inserted(model.get(), &QAbstractItemModel::rowsInserted);
    const QSignalSpy removed(model.get(), &QAbstractItemModel::rowsRemoved);

    transport->removePort("20:0");
    QCOMPARE(removed.count(), 1);
    QCOMPARE(removed.at(0).at(1).toInt(), 1);
    QCOMPARE(model->index(1).data().toString(), QStringLiteral("Synth"));

    transport->addPort("24:0", "LPD8"); // replugged, with a new id
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(inserted.at(0).at(1).toInt(), 2);
    QCOMPARE(model->index(2).data().toString(), QStringLiteral("LPD8"));
    QCOMPARE(reset.count(), 0);
}

void TestMidiConnectionsModel::test_connectPort()
{
    QSignalSpy connectedPort(model.get(), &MidiConnectionsModel::connectedPortChanged);
    model->connectPort(model->index(1));
    QCOMPARE(transport->connectedPort(), QString("20:0"));
    QCOMPARE(model->connectedPort(), model->index(1));
    QVERIFY(model->connected());
    QVERIFY(connectedPort.count() > 0);
    QCOMPARE(connectedPort.last().at(0).value<QModelIndex>(), model->index(1));

    // The device goes: the connection with it, and 'Disconnect' is current.
    transport->removePort("20:0");
    QVERIFY(!model->connected());
    QCOMPARE(model->connectedPort(), model->index(0));

    model->connectPort(model->index(1)); // now 'Synth'
    QCOMPARE(transport->connectedPort(), QString("28:0"));
    model->connectPort(model->index(0));
    QVERIFY(!transport->isConnected());
}

void TestMidiConnectionsModel::test_connectPortSignalsValidRange()
{
    const QSignalSpy spy(model.get(), &QAbstractItemModel::dataChanged);

    model->connectPort(model->index(0));

    QVERIFY(spy.count() >= 1);
    const auto topLeft = spy.last().at(0).value<QModelIndex>();
    const auto bottomRight = spy.last().at(1).value<QModelIndex>();
    QCOMPARE(topLeft.row(), 0);
    QCOMPARE(bottomRight.row(), model->rowCount() - 1);
}

void TestMidiConnectionsModel::test_otherToolsConnections()
{
    // One way only, made by another tool: managed outside the application.
    transport->setConnection("20:0", MidiPort::From);
    QVERIFY(model->connected());
    QVERIFY(model->externallyManaged());
    QVERIFY(!model->connectedPort().isValid());

    // Both ways by aconnect counts as the connected port, as it always did
    // (see the class comment): not externally managed.
    transport->setConnection("20:0", MidiPort::BothWays);
    QVERIFY(!model->externallyManaged());
    QCOMPARE(model->connectedPort(), model->index(1));
}

QTEST_GUILESS_MAIN(TestMidiConnectionsModel)

#include "midiconnectionsmodel_test.moc"
