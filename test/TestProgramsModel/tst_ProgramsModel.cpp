#include "programsmodel.h"

#include "db.h"

#include "test_sysex.h"

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

private:
    QSqlDatabase db;
    ProgramsModel* pm;
};

void TestProgramsModel::initTestCase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestProgramsModel::init() {
    QVERIFY(db.open());
    initDb(db);
    pm = new ProgramsModel(this, db);
}

void TestProgramsModel::cleanup() {
    pm->deleteLater();
    db.close();
}

void TestProgramsModel::test_data() {
    QTest::addColumn<int>("id");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("sysex");

    QTest::newRow("Force all") << 3 << "MyName" << allzero_sysex;
}

void TestProgramsModel::test() {
    QFETCH(int, id);
    QFETCH(QString, name);
    QFETCH(QByteArray, sysex);

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

QTEST_MAIN(TestProgramsModel)

#include "tst_ProgramsModel.moc"
