#include "db.h"

#include "test_sysex.h"

#include <QSqlDatabase>

#include <QtTest>

class TestDB : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_idsSequence();
    void test_fromSysex();

    void test_createProgram();
    void test_createProgram_data();

private:
    QSqlDatabase db;
};

void TestDB::initTestCase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestDB::init() {
    db.open();
    initDb(db);
}

void TestDB::cleanup() {
    db.close();
}

void TestDB::test_idsSequence() {
    QCOMPARE(createProgram(), 1);
    QCOMPARE(createProgram(), 2);
    QVERIFY(deleteProgram(2));
    QCOMPARE(createProgram(), 3);
}

void TestDB::test_fromSysex() {
    const int id = createProgram();
    QVERIFY(fromSysex(id, allzero_sysex));
    QCOMPARE(programSysex(id), allzero_sysex);
}

void TestDB::test_createProgram_data() {
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("sysex");
    QTest::addColumn<int>("id");

    QTest::addColumn<QString>("resName");
    QTest::addColumn<QByteArray>("resSysex");
    QTest::addColumn<int>("resId");

    QTest::addColumn<QString>("createProgramWarning");
    QTest::addColumn<QString>("programSysexWarning");

    QTest::newRow("All default")
            << QString()
            << QByteArray()
            << -1
            << "P1"
            << default_sysex
            << 1
            << QString()
            << QString();

    QTest::newRow("All specified")
            << "New program"
            << allzero_sysex
            << 10
            << "New program"
            << allzero_sysex
            << 10
            << QString()
            << QString();

    QTest::newRow("Short sysex")
            << QString()
            << invalid_sysex_short
            << -1
            << ""
            << QByteArray()
            << -1
            << "Failed to update program from sysex:  \" \""
            << "Failed to generate sysex for program -1 : \" \"";

    QTest::newRow("Out of bound value in sysex")
            << QString()
            << invalid_sysex_channel
            << -1
            << ""
            << QByteArray()
            << -1
            << "Failed to update program from sysex:  \"CHECK constraint failed: programs Unable to fetch row\""
            << "Failed to generate sysex for program -1 : \" \"";
}

void TestDB::test_createProgram() {
    QFETCH(QString, name);
    QFETCH(QByteArray, sysex);
    QFETCH(int, id);
    QFETCH(QString, resName);
    QFETCH(QByteArray, resSysex);
    QFETCH(int, resId);
    QFETCH(QString, createProgramWarning);
    QFETCH(QString, programSysexWarning);

    if (!createProgramWarning.isNull()) {
        QTest::ignoreMessage(QtWarningMsg, createProgramWarning.toLatin1());
    }
    QCOMPARE(createProgram(name, sysex, id), resId);

    QCOMPARE(programName(resId), resName);

    if (!programSysexWarning.isNull()) {
        QTest::ignoreMessage(QtWarningMsg, programSysexWarning.toLatin1());
    }
    QCOMPARE(programSysex(resId), resSysex);
}

QTEST_MAIN(TestDB)

#include "tst_db.moc"
