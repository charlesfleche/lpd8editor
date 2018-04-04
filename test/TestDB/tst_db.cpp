#include "db.h"

#include <QSqlDatabase>

#include <QtTest>

static const int default_sysex_size = 66;
static const char default_sysex[default_sysex_size] = {
(char)0xf0,
0x47,
0x7f, 0x75,
0x63, 0x00, 0x01, 0x01,
0x09,
0x24, 0x01, 0x30, 0x00,
0x25, 0x02, 0x31, 0x00,
0x26, 0x03, 0x32, 0x00,
0x27, 0x04, 0x33, 0x00,
0x28, 0x05, 0x10, 0x00,
0x29, 0x06, 0x11, 0x00,
0x2a, 0x07, 0x12, 0x00,
0x2b, 0x08, 0x13, 0x00,
0x10, 0x00, 0x7f,
0x11, 0x00, 0x7f,
0x12, 0x00, 0x7f,
0x13, 0x00, 0x7f,
0x30, 0x00, 0x7f,
0x31, 0x00, 0x7f,
0x32, 0x00, 0x7f,
0x33, 0x00, 0x7f,
(char)0xf7
};

static const char allzero_sysex[default_sysex_size] = {
(char)0xf0,
0x47,
0x7f, 0x75,
0x63, 0x00, 0x01, 0x01,
0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
(char)0xf7
};

static const char invalid_sysex_channel[default_sysex_size] = {
(char)0xf0,
0x47,
0x7f, 0x75,
0x63, 0x00, 0x01, 0x01,
0x16,                   // Bad MIDI channel
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
0x00, 0x00, 0x00,
(char)0xf7
};

static const int invalid_sysex_short_size = 6;
static const char invalid_sysex_short[invalid_sysex_short_size] = {
(char)0xf0,
0x47,
0x7f,
0x33,
0x00,
(char)0x7f
};

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
    const QByteArray sysex(allzero_sysex, default_sysex_size);
    QVERIFY(fromSysex(id, sysex));
    QCOMPARE(programSysex(id), sysex);
}

void TestDB::test_createProgram_data() {
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("sysex");
    QTest::addColumn<int>("id");

    QTest::addColumn<QString>("resName");
    QTest::addColumn<QByteArray>("resSysex");
    QTest::addColumn<int>("resId");

    QTest::newRow("All default")
            << QString()
            << QByteArray()
            << -1
            << "P1"
            << QByteArray((const char*)default_sysex, default_sysex_size)
            << 1;

    QTest::newRow("All specified")
            << "New program"
            << QByteArray(allzero_sysex, default_sysex_size)
            << 10
            << "New program"
            << QByteArray(allzero_sysex, default_sysex_size)
            << 10;

    QTest::newRow("Short sysex")
            << QString()
            << QByteArray(invalid_sysex_short, invalid_sysex_short_size)
            << -1
            << ""
            << QByteArray()
            << -1;

    QTest::newRow("Out of bound value in sysex")
            << QString()
            << QByteArray(invalid_sysex_channel, default_sysex_size)
            << -1
            << ""
            << QByteArray()
            << -1;
}

void TestDB::test_createProgram() {
    QFETCH(QString, name);
    QFETCH(QByteArray, sysex);
    QFETCH(int, id);
    QFETCH(QString, resName);
    QFETCH(QByteArray, resSysex);
    QFETCH(int, resId);

    QCOMPARE(createProgram(name, sysex, id), resId);
    QCOMPARE(programName(resId), resName);
    QCOMPARE(programSysex(resId), resSysex);
}

QTEST_MAIN(TestDB)

#include "tst_db.moc"
