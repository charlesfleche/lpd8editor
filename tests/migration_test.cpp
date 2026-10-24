#include "deviceprofile.h"
#include "migration.h"
#include "programstore.h"
#include "sqlite_schema.h"

#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using namespace lpd8;

namespace {

QByteArray readAll(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// Runs 'statements' on a new SQLite file at 'path', through a connection of
// its own, closed and removed afterwards (the migration must find the file
// as the application left it).
bool buildDatabase(const QString &path, const QStringList &statements, bool currentSchema)
{
    bool ok = true;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fixture");
        db.setDatabaseName(path);
        ok = db.open();
        if (ok && currentSchema) {
            // The schema the application creates today: CHECK constraints,
            // and triggers that insert the default pads and knobs.
            ok = createLastSqliteSchema(db);
        }
        QSqlQuery q(db);
        for (const QString &statement : statements) {
            if (ok && !q.exec(statement)) {
                qWarning() << statement << q.lastError().text();
                ok = false;
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("fixture");
    return ok;
}

// The schema of v0.0.0 to v0.0.5: the same tables, without CHECK
// constraints (git show v0.0.5:src/), and without the triggers' help here.
QStringList oldSchema()
{
    return {"create table programs(programId integer primary key, name varchar, channel integer)",
            "create table pads(programId integer NOT NULL, controlId integer NOT NULL, "
            "note integer NOT NULL, pc integer NOT NULL, cc integer NOT NULL, "
            "toggle integer NOT NULL, PRIMARY KEY (programId, controlId))",
            "create table knobs(programId integer NOT NULL, controlId integer NOT NULL, "
            "cc integer NOT NULL, low integer NOT NULL, high integer NOT NULL, "
            "PRIMARY KEY (programId, controlId))"};
}

// A complete, valid old program: 8 pads and 8 knobs with the factory values.
QStringList oldProgram(int id, const QString &name, const QString &channel = "9")
{
    QStringList statements{QStringLiteral("insert into programs values (%1, '%2', %3)")
                                   .arg(QString::number(id), name, channel)};
    const Program &factory = profile().defaultProgram;
    for (std::size_t i = 0; i < pad_count; ++i) {
        const QString controlId = QString::number(i + 1);
        statements << QStringLiteral("insert into pads values (%1, %2, %3, %4, %5, 0)")
                              .arg(QString::number(id),
                                   controlId,
                                   QString::number(factory.pads.at(i).note),
                                   QString::number(factory.pads.at(i).programChange),
                                   QString::number(factory.pads.at(i).controlChange));
    }
    for (std::size_t i = 0; i < knob_count; ++i) {
        statements << QStringLiteral("insert into knobs values (%1, %2, %3, 0, 127)")
                              .arg(QString::number(id),
                                   QString::number(i + 1),
                                   QString::number(factory.knobs.at(i).controlChange));
    }
    return statements;
}

} // namespace

class TestMigration : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_migratesTheCurrentSchema();
    void test_reportsBadRowsOfAnUnconstrainedDatabase();
    void test_nullNameBecomesEmpty();
    void test_doesNothingWhenTheJsonFileExists();
    void test_doesNothingWithoutADatabase();
    void test_unreadableDatabaseIsAnError();
    void test_hotJournalIsRecognised();
    void test_journalOfALiveWriteIsNotAHotJournal();
    void test_failedJsonWriteKeepsTheDatabase();
    void test_doesNotReplaceAnEarlierMigratedFile();
    void test_doesNotReplaceAnEarlierReport();
    void test_describeReport();

private:
    std::unique_ptr<QTemporaryDir> dir;
    QString sqlitePath;
    QString jsonPath;
};

void TestMigration::init()
{
    dir = std::make_unique<QTemporaryDir>();
    QVERIFY(dir->isValid());
    sqlitePath = dir->filePath("db.sqlite3");
    jsonPath = dir->filePath("programs.json");
}

void TestMigration::test_migratesTheCurrentSchema()
{
    // Ids 1, 2 and 5 (3 and 4 were deleted), with names, one edited value.
    QVERIFY(buildDatabase(
            sqlitePath,
            {"insert into programs (programId, name) values (1, 'Drums')",
             "insert into programs (programId, name) values (2, 'Café 日本語')",
             "insert into programs (programId, name, channel) values (5, 'Keys', 3)",
             "update pads set note = 60, toggle = 1 where programId = 2 and controlId = 4",
             "update knobs set high = 100 where programId = 5 and controlId = 8"},
            true));
    const QByteArray oldBytes = readAll(sqlitePath);

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    if (!result) { // not QVERIFY2: its message would be built even on success
        QFAIL(qPrintable(result.error().message));
    }
    const MigrationReport report = result.value().value();
    QCOMPARE(report.migrated, 3);
    QVERIFY(report.skipped.isEmpty());
    QCOMPARE(report.oldFileNowAt, sqlitePath + ".migrated");
    QVERIFY(report.renameError.isEmpty());
    QCOMPARE(report.reportFileAt, sqlitePath + ".migrated.txt");
    QVERIFY(report.reportFileError.isEmpty());

    // The old file is moved, byte for byte, and nothing is deleted.
    QVERIFY(!QFile::exists(sqlitePath));
    QCOMPARE(readAll(report.oldFileNowAt), oldBytes);
    // The migration's connection is gone, and the default one untouched.
    QVERIFY(!QSqlDatabase::contains("lpd8editor-migration"));
    QVERIFY(!QSqlDatabase::contains(QLatin1StringView(QSqlDatabase::defaultConnection)));

    ProgramStore store(jsonPath);
    QVERIFY(store.load().has_value());
    const QList<StoredProgram> &programs = store.programs();
    QCOMPARE(programs.size(), 3);
    // Ids, names and order are kept.
    QCOMPARE(programs.at(0).id, 1);
    QCOMPARE(programs.at(0).name, QStringLiteral("Drums"));
    QCOMPARE(programs.at(0).program, profile().defaultProgram);
    QCOMPARE(programs.at(1).id, 2);
    QCOMPARE(programs.at(1).name, QStringLiteral("Café 日本語"));
    QCOMPARE(programs.at(1).program.pads[3],
             (Pad{.note = 60, .programChange = 4, .controlChange = 51, .toggle = true}));
    QCOMPARE(programs.at(2).id, 5);
    QCOMPARE(programs.at(2).program.channel, quint8(3));
    QCOMPARE(programs.at(2).program.knobs[7].high, quint8(100));
    // New ids continue after the old ones.
    QCOMPARE(store.add(QStringLiteral("New"), profile().defaultProgram), std::optional<int>(6));
}

void TestMigration::test_reportsBadRowsOfAnUnconstrainedDatabase()
{
    QStringList statements = oldSchema() + oldProgram(1, "Good") + oldProgram(2, "Loud")
            + oldProgram(3, "Sticky") + oldProgram(4, "No channel", "NULL") + oldProgram(5, "Short")
            + oldProgram(6, "Wrapped") + oldProgram(7, "Huge") + oldProgram(8, "Half")
            + oldProgram(9, "Text");
    statements << "update pads set note = 300 where programId = 2 and controlId = 3"
               << "update pads set toggle = 2 where programId = 3 and controlId = 1"
               << "delete from knobs where programId = 5 and controlId = 8"
               // QVariant::toInt() would turn these into 40, -727379968 and 37,
               // reporting success.
               << "update pads set note = 4294967336 where programId = 6 and controlId = 2"
               << "update knobs set high = 1e12 where programId = 7 and controlId = 1"
               << "update pads set pc = 36.5 where programId = 8 and controlId = 5"
               << "update knobs set cc = 'abc' where programId = 9 and controlId = 4"
               // An id above INT_MAX: toInt() would turn it into 1, the 'Good'
               // program's id.
               << "insert into programs values (4294967297, 'Big id', 9)";
    QVERIFY(buildDatabase(sqlitePath, statements, false));

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    if (!result) { // not QVERIFY2: its message would be built even on success
        QFAIL(qPrintable(result.error().message));
    }
    const MigrationReport report = result.value().value();

    // The good program is migrated; each bad one is left out and reported,
    // by program and field. Nothing is wrapped (300 would become 44) or
    // dropped silently, and the data stays in the renamed database.
    QCOMPARE(report.migrated, 1);
    const QStringList expected{
            "program 2 (Loud): pad 3: note is 300, not an integer from 0 to 127",
            "program 3 (Sticky): pad 1: toggle is 2, not an integer from 0 to 1",
            "program 4 (No channel): channel is empty, not an integer from 0 to 15",
            "program 5 (Short): knob 8 is missing",
            "program 6 (Wrapped): pad 2: note is 4294967336, not an integer from 0 to 127",
            "program 7 (Huge): knob 1: high is 1000000000000, not an integer from 0 to 127",
            "program 8 (Half): pad 5: program change is 36.5, not an integer from 0 to 127",
            "program 9 (Text): knob 4: control change is 'abc', not an integer from 0 to 127",
            "program 4294967297 (Big id): its id is not an integer from 1 to 2147483646",
    };
    QCOMPARE(report.skipped, expected);
    QVERIFY(QFile::exists(report.oldFileNowAt));

    // A2: the report is saved next to the backup, so the list of skipped
    // programs survives a crash before the user reads it.
    QCOMPARE(report.reportFileAt, report.oldFileNowAt + ".txt");
    const QString saved = QString::fromUtf8(readAll(report.reportFileAt));
    QCOMPARE(saved, describe(report) + '\n');
    for (const QString &line : expected) {
        QVERIFY(saved.contains(line));
    }

    ProgramStore store(jsonPath);
    QVERIFY(store.load().has_value());
    QCOMPARE(store.programs().size(), 1);
    QCOMPARE(store.programs().at(0).name, QStringLiteral("Good"));
}

void TestMigration::test_nullNameBecomesEmpty()
{
    // v0.0.0 to v0.0.5 allowed a NULL name, which the old list showed like an
    // empty one: it migrates as an empty name.
    QStringList statements = oldSchema() + oldProgram(1, "x");
    statements << "update programs set name = NULL where programId = 1";
    QVERIFY(buildDatabase(sqlitePath, statements, false));

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    if (!result) { // not QVERIFY2: its message would be built even on success
        QFAIL(qPrintable(result.error().message));
    }
    QCOMPARE(result.value().value().migrated, 1);
    ProgramStore store(jsonPath);
    QVERIFY(store.load().has_value());
    QCOMPARE(store.programs().at(0).name, QString());
}

void TestMigration::test_doesNothingWhenTheJsonFileExists()
{
    // The state after a run that wrote the JSON file and stopped before
    // renaming the database: the next start must not migrate again, nor touch
    // either file.
    QVERIFY(buildDatabase(sqlitePath, oldSchema() + oldProgram(1, "Old"), false));
    ProgramStore existing(jsonPath);
    existing.add(QStringLiteral("Already migrated"), profile().defaultProgram);
    QVERIFY(existing.save().has_value());
    const QByteArray sqliteBytes = readAll(sqlitePath);
    const QByteArray jsonBytes = readAll(jsonPath);

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(result.has_value());
    QVERIFY(!result.value().has_value()); // nothing done
    QCOMPARE(readAll(sqlitePath), sqliteBytes);
    QCOMPARE(readAll(jsonPath), jsonBytes);
}

void TestMigration::test_doesNothingWithoutADatabase()
{
    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(result.has_value());
    QVERIFY(!result.value().has_value());
    QVERIFY(!QFile::exists(jsonPath));
}

void TestMigration::test_unreadableDatabaseIsAnError()
{
    {
        QFile f(sqlitePath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("this is not an SQLite database, but 64 bytes or more of text....");
    }
    const QByteArray bytes = readAll(sqlitePath);

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(!result.has_value());
    QVERIFY2(result.error().message.contains("Cannot read the old programs database"),
             qPrintable(result.error().message));
    QCOMPARE(result.error().kind, MigrationError::Kind::Other);
    QVERIFY(result.error().journalPath.isEmpty());
    QVERIFY(!QFile::exists(jsonPath));
    QCOMPARE(readAll(sqlitePath), bytes); // untouched, not renamed
}

void TestMigration::test_hotJournalIsRecognised()
{
    // A database a crash left in the middle of a write: the read-only
    // connection can't play its journal back, so the migration fails, and
    // says where the journal is, for the user's advice.
    const QString original = dir->filePath("original.sqlite3");
    QVERIFY(buildDatabase(original, oldSchema() + oldProgram(1, "Old"), false));
    QVERIFY(copyWithHotJournal(original, sqlitePath));
    const QByteArray bytes = readAll(sqlitePath);
    const QByteArray journal = readAll(sqlitePath + "-journal");

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().kind, MigrationError::Kind::HotJournal);
    QCOMPARE(result.error().journalPath, sqlitePath + "-journal");
    QVERIFY2(result.error().message.contains("Cannot read the old programs database"),
             qPrintable(result.error().message));
    // Nothing written, nothing played back.
    QVERIFY(!QFile::exists(jsonPath));
    QCOMPARE(readAll(sqlitePath), bytes);
    QCOMPARE(readAll(sqlitePath + "-journal"), journal);
}

void TestMigration::test_journalOfALiveWriteIsNotAHotJournal()
{
    // Another program is writing to the database right now: the journal is
    // there and not empty, as with a hot journal, but its writer holds the
    // lock. SQLite answers SQLITE_BUSY (after the driver's 5 second busy
    // timeout), not SQLITE_READONLY_ROLLBACK: the user must close that
    // program, not run sqlite3 on its database.
    QVERIFY(buildDatabase(sqlitePath, oldSchema() + oldProgram(1, "Old"), false));
    const InterruptedWrite write(sqlitePath);
    QVERIFY(write.ok());
    QVERIFY(QFileInfo(sqlitePath + "-journal").size() > 0);

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().kind, MigrationError::Kind::Other);
    QVERIFY(result.error().journalPath.isEmpty());
    QVERIFY(!QFile::exists(jsonPath));
}

void TestMigration::test_failedJsonWriteKeepsTheDatabase()
{
    QVERIFY(buildDatabase(sqlitePath, oldSchema() + oldProgram(1, "Old"), false));
    const QString notAFolder = dir->filePath("a-regular-file");
    {
        QFile f(notAFolder);
        QVERIFY(f.open(QIODevice::WriteOnly));
    }

    const auto result = migrateSqliteDatabase(sqlitePath, notAFolder + "/programs.json");
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().kind, MigrationError::Kind::Other);
    QVERIFY(QFile::exists(sqlitePath)); // not renamed: the next start retries
    QVERIFY(!QFile::exists(sqlitePath + ".migrated"));
}

void TestMigration::test_doesNotReplaceAnEarlierMigratedFile()
{
    QVERIFY(buildDatabase(sqlitePath, oldSchema() + oldProgram(1, "Old"), false));
    {
        QFile f(sqlitePath + ".migrated");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("an earlier migration");
    }

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(result.has_value());
    const MigrationReport report = result.value().value();
    QVERIFY(report.oldFileNowAt.startsWith(sqlitePath + ".migrated-"));
    QCOMPARE(readAll(sqlitePath + ".migrated"), QByteArray("an earlier migration"));
    // The report follows the backup's name, and leaves the earlier one's
    // place alone.
    QCOMPARE(report.reportFileAt, report.oldFileNowAt + ".txt");
    QVERIFY(QFile::exists(report.reportFileAt));
    QVERIFY(!QFile::exists(sqlitePath + ".migrated.txt"));
}

void TestMigration::test_doesNotReplaceAnEarlierReport()
{
    // The backup of an earlier migration was deleted, but not its report:
    // the report is kept, and the new backup gets a name of its own.
    QVERIFY(buildDatabase(sqlitePath, oldSchema() + oldProgram(1, "Old"), false));
    {
        QFile f(sqlitePath + ".migrated.txt");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("an earlier report");
    }

    const auto result = migrateSqliteDatabase(sqlitePath, jsonPath);
    QVERIFY(result.has_value());
    const MigrationReport report = result.value().value();
    QVERIFY(report.oldFileNowAt.startsWith(sqlitePath + ".migrated-"));
    QCOMPARE(report.reportFileAt, report.oldFileNowAt + ".txt");
    QCOMPARE(readAll(sqlitePath + ".migrated.txt"), QByteArray("an earlier report"));
    QVERIFY(!QFile::exists(sqlitePath + ".migrated"));
}

void TestMigration::test_describeReport()
{
    const MigrationReport full{.migrated = 1,
                               .skipped = {QStringLiteral("program 4 (Bass): channel is 20")},
                               .oldFileNowAt = QStringLiteral("/data/db.sqlite3.migrated"),
                               .renameError = {},
                               .reportFileAt = QStringLiteral("/data/db.sqlite3.migrated.txt"),
                               .reportFileError = {}};
    const QString text = describe(full);
    QVERIFY(text.contains(QStringLiteral("1 program(s) moved")));
    QVERIFY(text.contains(QStringLiteral("program 4 (Bass): channel is 20")));
    QVERIFY(text.contains(QDir::toNativeSeparators(QStringLiteral("/data/db.sqlite3.migrated."))));
    QVERIFY(text.contains(
            QDir::toNativeSeparators(QStringLiteral("/data/db.sqlite3.migrated.txt."))));

    const MigrationReport notRenamed{.migrated = 2,
                                     .skipped = {},
                                     .oldFileNowAt = {},
                                     .renameError = QStringLiteral("Cannot rename a to b."),
                                     .reportFileAt = {},
                                     .reportFileError = QStringLiteral("Cannot save c.")};
    const QString text2 = describe(notRenamed);
    QVERIFY(text2.contains(QStringLiteral("Cannot rename a to b.")));
    QVERIFY(text2.contains(QStringLiteral("won't be read again")));
    QVERIFY(text2.contains(QStringLiteral("Cannot save c.")));

    // An old database with no programs: still a notice, since the database
    // was moved, but not "0 program(s) moved".
    const MigrationReport empty{.migrated = 0,
                                .skipped = {},
                                .oldFileNowAt = QStringLiteral("/data/db.sqlite3.migrated"),
                                .renameError = {},
                                .reportFileAt = {},
                                .reportFileError = {}};
    const QString text3 = describe(empty);
    QVERIFY(text3.contains(QStringLiteral("held no programs")));
    QVERIFY(!text3.contains(QStringLiteral("0 program")));
    QVERIFY(text3.contains(QDir::toNativeSeparators(QStringLiteral("/data/db.sqlite3.migrated"))));
}

QTEST_GUILESS_MAIN(TestMigration)

#include "migration_test.moc"
