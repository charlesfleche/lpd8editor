#include "iomidi.h"
#include "midiconnectionsmodel.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QtTest>

class TestMidiConnectionsModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void test_modelContract();
    void test_connectPortSignalsValidRange();

private:
    IOMidi *io{};
    QByteArray skipReason;
};

void TestMidiConnectionsModel::initTestCase()
{
    try {
        io = new IOMidi(this);
    } catch (const std::exception &e) {
        // Not an error: CI machines usually have no /dev/snd/seq.
        skipReason = QByteArray("No ALSA sequencer available: ") + e.what();
    }
}

void TestMidiConnectionsModel::init()
{
    if (!io) {
        QSKIP(skipReason.constData());
    }
}

void TestMidiConnectionsModel::test_modelContract()
{
    MidiConnectionsModel model(io);
    // Checks the QAbstractItemModel rules on every change of the model, and
    // fails the test on the first violation.
    const QAbstractItemModelTester tester(&model,
                                          QAbstractItemModelTester::FailureReportingMode::QtTest);

    // Row 0 is the 'Disconnect' choice.
    model.connectPort(model.index(0));
}

void TestMidiConnectionsModel::test_connectPortSignalsValidRange()
{
    MidiConnectionsModel model(io);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);

    model.connectPort(model.index(0));

    QCOMPARE(spy.count(), 1);
    const auto topLeft = spy.at(0).at(0).value<QModelIndex>();
    const auto bottomRight = spy.at(0).at(1).value<QModelIndex>();
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QCOMPARE(bottomRight.row(), model.rowCount(QModelIndex()) - 1);
}

QTEST_GUILESS_MAIN(TestMidiConnectionsModel)

#include "midiconnectionsmodel_test.moc"
