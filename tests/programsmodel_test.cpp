#include "programsmodel.h"

#include "db.h"

#include "test_sysex.h"

#include <QAbstractItemModelTester>
#include <QSqlDatabase>

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

QTEST_GUILESS_MAIN(TestProgramsModel)

#include "programsmodel_test.moc"
