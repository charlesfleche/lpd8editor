#include "db.h"
#include "deviceprofile.h"
#include "lpd8codec.h"
#include "utils.h"

#include "test_sysex.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTemporaryFile>

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
    void test_fromSysexForMissingProgramFails();
    void test_factoryProgramMatchesTheSqlTrigger();
    void test_programSysexRejectsOldUnconstrainedValues_data();
    void test_programSysexRejectsOldUnconstrainedValues();
    void test_programSysexFailsOnMissingRows();

    void test_exportImportRoundTrip();

    void test_initDbReportsErrors();
    void test_openDatabaseCreatesFolderAndSchema();
    void test_openDatabaseReportsErrors();
    void test_initDbIsAllOrNothing();

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
    // One byte short: the last knob's 'high' byte is missing. QByteArray
    // keeps a '\0' after its data, so reading one byte too far returns 0,
    // a valid value: only a size check catches this.
    QTest::newRow("Truncated by one byte") << default_sysex.first(default_sysex.size() - 1);
    QByteArray otherManufacturer = default_sysex;
    otherManufacturer[1] = '\x41';
    QTest::newRow("Not from Akai") << otherManufacturer;
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

void TestDB::test_fromSysexForMissingProgramFails()
{
    // An UPDATE that matches no row is not an SQL error, so this used to
    // 'succeed' without changing anything.
    QVERIFY(programIds().isEmpty());
    QVERIFY(!fromSysex(42, default_sysex));
}

void TestDB::test_factoryProgramMatchesTheSqlTrigger()
{
    // A new program in the database gets its values from the SQL trigger
    // (create_trigger_add_program.sql). lpd8::profile().defaultProgram must
    // be the same program, compared on all 66 bytes of the stored message.
    const int id = createProgram();
    QCOMPARE(programSysex(id), lpd8::encodeStoredProgram(lpd8::profile().defaultProgram));
}

void TestDB::test_programSysexRejectsOldUnconstrainedValues_data()
{
    QTest::addColumn<QString>("update");

    QTest::newRow("Note 300") << "update pads set note = 300 where controlId = 3";
    QTest::newRow("Negative note") << "update pads set note = -128 where controlId = 3";
    QTest::newRow("Toggle 2") << "update pads set toggle = 2 where controlId = 3";
    QTest::newRow("Channel 16") << "update programs set channel = 16";
    QTest::newRow("Knob high 128") << "update knobs set high = 128 where controlId = 8";
    // QVariant::toInt() wraps or rounds these, and still says 'ok': 4294967336
    // would become 40 and 36.5 would become 37, silently.
    QTest::newRow("Note 2^32 + 40") << "update pads set note = 4294967336 where controlId = 3";
    QTest::newRow("Note 1e12") << "update pads set note = 1e12 where controlId = 3";
    QTest::newRow("Note 36.5") << "update pads set note = 36.5 where controlId = 3";
    QTest::newRow("Note 'abc'") << "update pads set note = 'abc' where controlId = 3";
    QTest::newRow("Toggle 4294967297") << "update pads set toggle = 4294967297 where controlId = 3";
}

void TestDB::test_programSysexRejectsOldUnconstrainedValues()
{
    // NOLINTNEXTLINE(misc-const-correctness): QFETCH declares the variable
    QFETCH(QString, update);

    // Databases created before 2018 (v0.0.0 to v0.0.5) have these tables
    // without CHECK constraints (see 'git show v0.0.5:src/'), and SQLite
    // never adds constraints to an existing table. Rebuild that schema, with
    // one complete program, then plant a value the LPD8 can't take.
    QSqlQuery q;
    for (const char *statement : {
                 "drop trigger aft_insert",
                 "drop trigger aft_delete",
                 "drop table knobs",
                 "drop table pads",
                 "drop table programs",
                 "create table programs(programId integer primary key, name varchar, channel "
                 "integer)",
                 "create table pads(programId integer NOT NULL, controlId integer NOT NULL, "
                 "note integer NOT NULL, pc integer NOT NULL, cc integer NOT NULL, "
                 "toggle integer NOT NULL, PRIMARY KEY (programId, controlId))",
                 "create table knobs(programId integer NOT NULL, controlId integer NOT NULL, "
                 "cc integer NOT NULL, low integer NOT NULL, high integer NOT NULL, "
                 "PRIMARY KEY (programId, controlId))",
                 "insert into programs values (1, 'Old', 9)",
         }) {
        QVERIFY2(q.exec(statement), qPrintable(q.lastError().text()));
    }
    for (int controlId = 1; controlId <= 8; ++controlId) {
        QVERIFY(q.exec(
                QStringLiteral("insert into pads values (1, %1, 36, 1, 48, 0)").arg(controlId)));
        QVERIFY(q.exec(
                QStringLiteral("insert into knobs values (1, %1, 16, 0, 127)").arg(controlId)));
    }
    QVERIFY(!programSysex(1).isEmpty()); // valid as it is

    QVERIFY2(q.exec(update), qPrintable(q.lastError().text()));
    // Reported, not wrapped (300 would become 44) or normalized (toggle 2
    // would become 'on').
    QCOMPARE(programSysex(1), QByteArray());
}

void TestDB::test_programSysexFailsOnMissingRows()
{
    QCOMPARE(programSysex(42), QByteArray()); // no such program

    const int id = createProgram();
    QSqlQuery q;
    QVERIFY(q.exec(
            QStringLiteral("delete from pads where programId = %1 and controlId = 3").arg(id)));
    QCOMPARE(programSysex(id), QByteArray());

    const int other = createProgram();
    QVERIFY(q.exec(
            QStringLiteral("delete from knobs where programId = %1 and controlId = 8").arg(other)));
    QCOMPARE(programSysex(other), QByteArray());
}

void TestDB::test_exportImportRoundTrip()
{
    // A file exported by the application must import again: the stricter
    // validation must accept the application's own format.
    const int id = createProgram(QStringLiteral("Exported"), allzero_sysex);
    const QByteArray exported = programSysex(id);

    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();
    QVERIFY(writeProgramFile(exported, file.fileName()).has_value());

    const auto imported = fromSysexTextFile(file.fileName());
    QVERIFY(imported.has_value());
    QCOMPARE(*imported, exported);
    const int importedId = createProgram(QStringLiteral("Imported"), *imported);
    QVERIFY(importedId != -1);
    QCOMPARE(programSysex(importedId), exported);
}

void TestDB::test_initDbReportsErrors()
{
    // An empty file is an empty SQLite database. Opened read-only, creating
    // the schema must fail, and initDb() must say so.
    QTemporaryFile file;
    QVERIFY(file.open());
    file.close();
    {
        QSqlDatabase readOnly = QSqlDatabase::addDatabase("QSQLITE", "readonly");
        readOnly.setDatabaseName(file.fileName());
        readOnly.setConnectOptions("QSQLITE_OPEN_READONLY");
        QVERIFY(readOnly.open());
        QVERIFY(initDb(readOnly).isValid());
        readOnly.close();
    }
    QSqlDatabase::removeDatabase("readonly");
}

void TestDB::test_openDatabaseCreatesFolderAndSchema()
{
    const QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("not/yet/there/db.sqlite3");
    {
        const auto opened = openDatabase(path, QLatin1StringView("open"));
        // Not QVERIFY2(opened, opened.error()): its message is evaluated even
        // when the check passes, and error() on a value is undefined.
        if (!opened) {
            QFAIL(qPrintable(opened.error()));
        }
        QVERIFY(opened->tables().contains("programs"));
    }
    QSqlDatabase::removeDatabase("open");
}

void TestDB::test_openDatabaseReportsErrors()
{
    // A regular file can't contain a folder, so the database's folder
    // can't be created.
    QTemporaryFile file;
    QVERIFY(file.open());
    const QString path = file.fileName() + "/sub/db.sqlite3";

    const auto opened = openDatabase(path, QLatin1StringView("unopenable"));
    QVERIFY(!opened.has_value());
    QVERIFY2(opened.error().contains("Cannot create the folder"), qPrintable(opened.error()));
    QSqlDatabase::removeDatabase("unopenable");
}

void TestDB::test_initDbIsAllOrNothing()
{
    {
        QSqlDatabase other = QSqlDatabase::addDatabase("QSQLITE", "partial");
        other.setDatabaseName(":memory:");
        QVERIFY(other.open());
        // A table named like the third table of the schema makes its creation
        // fail after 'programs' and 'pads' were created.
        QSqlQuery q(other);
        QVERIFY(q.exec("create table knobs(x integer)"));

        QVERIFY(initDb(other).isValid());
        // Rolled back: no half-created schema that would later pass for
        // initialized (isInitialized() only looks for 'programs').
        QVERIFY(!other.tables().contains("programs"));
        other.close();
    }
    QSqlDatabase::removeDatabase("partial");
}

QTEST_GUILESS_MAIN(TestDB)

#include "db_test.moc"
