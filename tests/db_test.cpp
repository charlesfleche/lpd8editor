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

    void test_failedCreateProgramLeavesNothing_data();
    void test_failedCreateProgramLeavesNothing();
    void test_failedFromSysexChangesNothing();

private:
    QSqlDatabase db;
};

void TestDB::initTestCase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestDB::init()
{
    db.open();
    initDb(db);
}

void TestDB::cleanup()
{
    db.close();
}

void TestDB::test_idsSequence()
{
    QCOMPARE(createProgram(), 1);
    QCOMPARE(createProgram(), 2);
    QVERIFY(deleteProgram(2));
    QCOMPARE(createProgram(), 3);
}

void TestDB::test_fromSysex()
{
    const int id = createProgram();
    QVERIFY(fromSysex(id, allzero_sysex));
    QCOMPARE(programSysex(id), allzero_sysex);
}

void TestDB::test_createProgram_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("sysex");
    QTest::addColumn<int>("id");

    QTest::addColumn<QString>("resName");
    QTest::addColumn<QByteArray>("resSysex");
    QTest::addColumn<int>("resId");

    QTest::addColumn<bool>("expectProgramWarning");
    QTest::addColumn<bool>("expectSysexWarning");

    QTest::newRow("All default") << QString() << QByteArray() << -1 << "P1" << default_sysex << 1
                                 << false << false;

    QTest::newRow("All specified") << "New program" << allzero_sysex << 10 << "New program"
                                   << allzero_sysex << 10 << false << false;

    QTest::newRow("Short sysex") << QString() << invalid_sysex_short << -1 << "" << QByteArray()
                                 << -1 << true << true;

    QTest::newRow("Out of bound value in sysex")
            << QString() << invalid_sysex_channel << -1 << "" << QByteArray() << -1 << true << true;
}

void TestDB::test_createProgram()
{
    // QFETCH declares the variable itself, so it cannot be made const.
    // NOLINTBEGIN(misc-const-correctness)
    QFETCH(QString, name);
    QFETCH(QByteArray, sysex);
    QFETCH(int, id);
    QFETCH(QString, resName);
    QFETCH(QByteArray, resSysex);
    QFETCH(int, resId);
    QFETCH(bool, expectProgramWarning);
    QFETCH(bool, expectSysexWarning);
    // NOLINTEND(misc-const-correctness)

    if (expectProgramWarning) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
    }
    QCOMPARE(createProgram(name, sysex, id), resId);

    QCOMPARE(programName(resId), resName);

    if (expectSysexWarning) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
    }
    QCOMPARE(programSysex(resId), resSysex);
}

void TestDB::test_failedCreateProgramLeavesNothing_data()
{
    QTest::addColumn<QByteArray>("sysex");

    QTest::newRow("Short sysex") << invalid_sysex_short;
    QTest::newRow("Out of bound channel") << invalid_sysex_channel;
}

void TestDB::test_failedCreateProgramLeavesNothing()
{
    // NOLINTNEXTLINE(misc-const-correctness): QFETCH declares the variable
    QFETCH(QByteArray, sysex);

    QCOMPARE(createProgram(QString(), sysex), -1);
    // The failed creation must be rolled back: no half-created program.
    QCOMPARE(programIds(), QList<int>());
}

void TestDB::test_failedFromSysexChangesNothing()
{
    const int id = createProgram();
    const QByteArray before = programSysex(id);

    // Valid up to pad 8, whose note (byte 37) is out of range: the update
    // fails after pads 1 to 7 were already written.
    QByteArray sysex = allzero_sysex;
    sysex[37] = '\x80';

    QVERIFY(!fromSysex(id, sysex));
    QCOMPARE(programSysex(id), before);
}

QTEST_GUILESS_MAIN(TestDB)

#include "db_test.moc"
