#include "db.h"
#include "deviceprofile.h"
#include "migration.h"
#include "programsdocument.h"
#include "programstore.h"
#include "startup.h"

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

void writeFile(const QString &path, const QByteArray &content)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    QCOMPARE(f.write(content), content.size());
}

// A database with today's schema (its triggers add the default pads and
// knobs) and two programs, through a connection of its own, closed and
// removed afterwards.
bool buildDatabase(const QString &path)
{
    bool ok = true;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fixture");
        db.setDatabaseName(path);
        ok = db.open() && !initDb(db).isValid();
        QSqlQuery q(db);
        for (const char *statement :
             {"insert into programs (programId, name, channel) values (3, 'Drums', 9)",
              "insert into programs (programId, name, channel) values (7, 'Keys', 2)"}) {
            if (ok && !q.exec(QString::fromLatin1(statement))) {
                qWarning() << statement << q.lastError().text();
                ok = false;
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("fixture");
    return ok;
}

} // namespace

class TestStartup : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_firstStartIsEmptyAndWritesNothing();
    void test_migrationIsReportedAndLoaded();
    void test_unreadableDatabaseIsRefused();
    void test_unreadableStoreIsRefused();
    void test_loadFailureAfterMigrationKeepsTheReport();

private:
    std::unique_ptr<QTemporaryDir> dir;
    QString sqlitePath;
    QString jsonPath;
};

void TestStartup::init()
{
    dir = std::make_unique<QTemporaryDir>();
    sqlitePath = dir->filePath("db.sqlite3");
    jsonPath = dir->filePath("programs.json");
}

void TestStartup::test_firstStartIsEmptyAndWritesNothing()
{
    ProgramsDocument document(jsonPath);
    const auto opened = openPrograms(document, sqlitePath);
    QVERIFY(opened.has_value());
    QVERIFY(!opened->has_value());
    QVERIFY(document.store().programs().isEmpty());

    // The file is created by the first edit, not by starting.
    QCoreApplication::processEvents();
    QVERIFY(!QFile::exists(jsonPath));
    QVERIFY(!QFile::exists(sqlitePath));
}

void TestStartup::test_migrationIsReportedAndLoaded()
{
    QVERIFY(buildDatabase(sqlitePath));

    ProgramsDocument document(jsonPath);
    const auto opened = openPrograms(document, sqlitePath);
    QVERIFY(opened.has_value());
    QVERIFY(opened->has_value());
    QVERIFY(opened->value().contains(QStringLiteral("2 program(s) moved")));
    QCOMPARE(opened->value(),
             describe(MigrationReport{.migrated = 2,
                                      .skipped = {},
                                      .oldFileNowAt = sqlitePath + ".migrated",
                                      .renameError = {},
                                      .reportFileAt = sqlitePath + ".migrated.txt",
                                      .reportFileError = {}}));

    // The document holds exactly what the migration wrote: the same
    // programs, ids, names and order, read back by a store of its own.
    ProgramStore written(jsonPath);
    QVERIFY(written.load().has_value());
    QCOMPARE(document.store().programs(), written.programs());
    QCOMPARE(document.store().programs().size(), 2);
    QCOMPARE(document.store().programs().at(0).id, 3);
    QCOMPARE(document.store().programs().at(0).name, QStringLiteral("Drums"));
    QCOMPARE(document.store().programs().at(0).program.channel, 9);
    QCOMPARE(document.store().programs().at(1).id, 7);

    // Loading wrote nothing more, and there is nothing to undo.
    const QByteArray json = readAll(jsonPath);
    QCoreApplication::processEvents();
    QCOMPARE(readAll(jsonPath), json);
    QCOMPARE(document.undoStack().count(), 0);
}

void TestStartup::test_unreadableDatabaseIsRefused()
{
    const QByteArray garbage("this is not an SQLite database, but it is in its place");
    writeFile(sqlitePath, garbage);

    ProgramsDocument document(jsonPath);
    const auto opened = openPrograms(document, sqlitePath);
    QVERIFY(!opened.has_value());
    const QString &message = opened.error();
    QVERIFY(message.contains(QDir::toNativeSeparators(sqlitePath)));
    QVERIFY(message.contains(QStringLiteral("Nothing was changed")));

    QCoreApplication::processEvents();
    QCOMPARE(readAll(sqlitePath), garbage);
    QVERIFY(!QFile::exists(jsonPath));
}

void TestStartup::test_unreadableStoreIsRefused()
{
    // A file from a newer version, and an old database that must not be
    // migrated again over it.
    const QByteArray newer(R"({"format": 99, "programs": []})");
    writeFile(jsonPath, newer);
    QVERIFY(buildDatabase(sqlitePath));
    const QByteArray sqlite = readAll(sqlitePath);

    ProgramsDocument document(jsonPath);
    const auto opened = openPrograms(document, sqlitePath);
    QVERIFY(!opened.has_value());
    const QString &message = opened.error();
    QVERIFY(message.contains(QDir::toNativeSeparators(jsonPath)));
    QVERIFY(message.contains(QStringLiteral("newer version")));
    QVERIFY(message.contains(QStringLiteral("Nothing was changed")));

    // Even a change and a save by mistake leave both files as they were.
    QVERIFY(document.store().add(QStringLiteral("A"), profile().defaultProgram).has_value());
    QVERIFY(!document.saveNow());
    QCoreApplication::processEvents();
    QCOMPARE(readAll(jsonPath), newer);
    QCOMPARE(readAll(sqlitePath), sqlite);
    QVERIFY(!QFile::exists(sqlitePath + ".migrated"));
}

void TestStartup::test_loadFailureAfterMigrationKeepsTheReport()
{
    // A1: the migration ran (it wrote the file and moved the database), and
    // then the file can't be loaded. The refusal must not claim that nothing
    // changed, and must carry the report, which no later start shows.
    const QByteArray newer(R"({"format": 99, "programs": []})");
    writeFile(jsonPath, newer);
    const MigrationReport report{.migrated = 1,
                                 .skipped = {QStringLiteral("program 4 (Bass): channel is 20")},
                                 .oldFileNowAt = sqlitePath + ".migrated",
                                 .renameError = {},
                                 .reportFileAt = sqlitePath + ".migrated.txt",
                                 .reportFileError = {}};

    ProgramsDocument document(jsonPath);
    const auto loaded = loadPrograms(document, report);
    QVERIFY(!loaded.has_value());
    const QString &message = loaded.error();
    QVERIFY(!message.contains(QStringLiteral("Nothing was changed")));
    QVERIFY(message.contains(describe(report)));
    QVERIFY(message.contains(QStringLiteral("newer version")));
    QVERIFY(message.contains(QDir::toNativeSeparators(jsonPath)));
    QCOMPARE(readAll(jsonPath), newer);
}

QTEST_GUILESS_MAIN(TestStartup)
#include "startup_test.moc"
