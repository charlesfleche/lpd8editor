#include "programsmodel.h"

#include "commands.h"
#include "db.h"

#include "test_sysex.h"

#include <QAbstractItemModelTester>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QUndoStack>

#include <QtTest>

class TestProgramsModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_data();
    void test();

    void test_modelContract();

    void test_columnsMatchTheSchema();

    void test_setData_data();
    void test_setData();

private:
    QSqlDatabase db;
    ProgramsModel *pm{};
};

void TestProgramsModel::initTestCase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestProgramsModel::init()
{
    QVERIFY(db.open());
    initDb(db);
    pm = new ProgramsModel(this, db);
}

void TestProgramsModel::cleanup()
{
    pm->deleteLater();
    db.close();
}

void TestProgramsModel::test_data()
{
    QTest::addColumn<int>("id");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("sysex");

    QTest::newRow("Force all") << 3 << "MyName" << allzero_sysex;
}

void TestProgramsModel::test()
{
    // QFETCH declares the variable itself, so it cannot be made const.
    // NOLINTBEGIN(misc-const-correctness)
    QFETCH(int, id);
    QFETCH(QString, name);
    QFETCH(QByteArray, sysex);
    // NOLINTEND(misc-const-correctness)

    QCOMPARE(pm->rowCount(), 0);

    QCOMPARE(pm->createProgram(name, sysex, id), id);
    QCOMPARE(pm->rowCount(), 1);
    QCOMPARE(pm->programIndex(id), pm->index(0, 0));
    QCOMPARE(pm->programName(id), name);
    QCOMPARE(pm->programSysex(id), sysex);
    QVERIFY(pm->updateProgramFromSysex(id, default_sysex));
    QCOMPARE(pm->programSysex(id), default_sysex);
    QVERIFY(pm->deleteProgram(id));
    QCOMPARE(pm->rowCount(), 0);
}

void TestProgramsModel::test_modelContract()
{
    // QAbstractItemModelTester walks the whole tree when attached, then
    // checks every change the model signals, and fails the test on the
    // first violation of the QAbstractItemModel rules.
    pm->createProgram(QStringLiteral("First"));
    pm->createProgram(QStringLiteral("Second"));
    const QAbstractItemModelTester tester(pm,
                                          QAbstractItemModelTester::FailureReportingMode::QtTest);

    const int id = pm->createProgram(QStringLiteral("Third"));
    QVERIFY(pm->updateProgramFromSysex(id, allzero_sysex));
    QVERIFY(pm->deleteProgram(id));
}

void TestProgramsModel::test_columnsMatchTheSchema()
{
    // The column constants must follow the order of the SQL tables'
    // columns: check each one against the real schema.
    const QSqlRecord programs = db.record("programs");
    QCOMPARE(programs.indexOf("programId"), ProgramColumn::ProgramId);
    QCOMPARE(programs.indexOf("name"), ProgramColumn::Name);
    QCOMPARE(programs.indexOf("channel"), ProgramColumn::Channel);

    const QSqlRecord pads = db.record("pads");
    QCOMPARE(pads.indexOf("programId"), PadColumn::ProgramId);
    QCOMPARE(pads.indexOf("controlId"), PadColumn::ControlId);
    QCOMPARE(pads.indexOf("note"), PadColumn::Note);
    QCOMPARE(pads.indexOf("pc"), PadColumn::ProgramChange);
    QCOMPARE(pads.indexOf("cc"), PadColumn::ControlChange);
    QCOMPARE(pads.indexOf("toggle"), PadColumn::Toggle);

    const QSqlRecord knobs = db.record("knobs");
    QCOMPARE(knobs.indexOf("programId"), KnobColumn::ProgramId);
    QCOMPARE(knobs.indexOf("controlId"), KnobColumn::ControlId);
    QCOMPARE(knobs.indexOf("cc"), KnobColumn::ControlChange);
    QCOMPARE(knobs.indexOf("low"), KnobColumn::Low);
    QCOMPARE(knobs.indexOf("high"), KnobColumn::High);
}

void TestProgramsModel::test_setData_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<int>("role");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<bool>("undoable");

    // The default note of pad 1 is 36 (C1).
    QTest::newRow("New value") << QVariant(60) << int(Qt::EditRole) << true << true;
    QTest::newRow("Same value") << QVariant(36) << int(Qt::EditRole) << true << false;
    QTest::newRow("Above the MIDI range") << QVariant(128) << int(Qt::EditRole) << false << false;
    QTest::newRow("Negative") << QVariant(-1) << int(Qt::EditRole) << false << false;
    QTest::newRow("Not a number") << QVariant("C1") << int(Qt::EditRole) << false << false;
    QTest::newRow("Display role") << QVariant(60) << int(Qt::DisplayRole) << false << false;
}

void TestProgramsModel::test_setData()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QVariant, value);
    QFETCH(int, role);
    QFETCH(bool, accepted);
    QFETCH(bool, undoable);
    // NOLINTEND(misc-const-correctness)

    const int id = pm->createProgram();
    const QModelIndex pads = pm->padsParentIndex(pm->programIndex(id));
    const QModelIndex note = pm->index(0, PadColumn::Note, pads);
    QCOMPARE(note.data(Qt::EditRole).toInt(), 36);

    const int undoCount = undoStack()->count();
    QCOMPARE(pm->setData(note, value, role), accepted);
    QCOMPARE(undoStack()->count(), undoCount + (undoable ? 1 : 0));
    QCOMPARE(note.data(Qt::EditRole), accepted ? value : QVariant(36));
}

QTEST_GUILESS_MAIN(TestProgramsModel)

#include "programsmodel_test.moc"
