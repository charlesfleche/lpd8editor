#include "db.h"
#include "deviceprofile.h"
#include "programfile.h"
#include "utils.h"

#include <QFile>
#include <QSqlDatabase>
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

// The program in tests/fixtures/exported-program.lpd8 (see the README
// there): the factory program, with a few values changed.
Program fixtureProgram()
{
    Program p = profile().defaultProgram;
    p.channel = 3; // channel 4, as the window shows it
    p.pads[0].note = 60;
    p.pads[0].toggle = true;
    p.knobs[7].high = 100;
    return p;
}

} // namespace

class TestProgramFile : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_importsTheFixture();
    void test_exportIsTheFixture();
    void test_sameFileAsTheSqlitePath();
    void test_nameIsTheFileName();
    void test_notAProgramIsRefused();
    void test_valueOutOfRangeIsRefused();
    void test_exportFailureIsReported();

private:
    std::unique_ptr<QTemporaryDir> dir;
    QString fixture;
};

void TestProgramFile::init()
{
    dir = std::make_unique<QTemporaryDir>();
    fixture = QFINDTESTDATA("fixtures/exported-program.lpd8");
    QVERIFY(!fixture.isEmpty());
}

void TestProgramFile::test_importsTheFixture()
{
    const auto imported = importProgramFile(fixture);
    QVERIFY(imported.has_value());
    QCOMPARE(imported->name, QStringLiteral("exported-program"));
    QCOMPARE(imported->program, fixtureProgram());
}

void TestProgramFile::test_exportIsTheFixture()
{
    // The fixture was written by the SQLite version: an export from the
    // store must be the same file, byte for byte (no behaviour change).
    const QString path = dir->filePath("out.lpd8");
    QVERIFY(exportProgramFile(fixtureProgram(), path).has_value());
    QCOMPARE(readAll(path), readAll(fixture));
}

void TestProgramFile::test_sameFileAsTheSqlitePath()
{
    // While both exist (until item 4.7b): the old import and export, through
    // the database, and the new ones give the same file.
    QString oldPath;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(!initDb(db).isValid());

        const auto sysex = fromSysexTextFile(fixture);
        QVERIFY(sysex.has_value());
        const int id = createProgram(QStringLiteral("exported-program"));
        QVERIFY(fromSysex(id, *sysex));
        oldPath = dir->filePath("old.lpd8");
        QVERIFY(writeProgramFile(programSysex(id), oldPath).has_value());
        db.close();
    }
    QSqlDatabase::removeDatabase(QLatin1StringView(QSqlDatabase::defaultConnection));

    const auto imported = importProgramFile(fixture);
    QVERIFY(imported.has_value());
    const QString newPath = dir->filePath("new.lpd8");
    QVERIFY(exportProgramFile(imported->program, newPath).has_value());

    QCOMPARE(readAll(newPath), readAll(oldPath));
    QCOMPARE(readAll(newPath), readAll(fixture));
}

void TestProgramFile::test_nameIsTheFileName()
{
    // Only the last extension goes, as before.
    const QString path = dir->filePath("Drums.v2.lpd8");
    QVERIFY(exportProgramFile(fixtureProgram(), path).has_value());
    const auto imported = importProgramFile(path);
    QVERIFY(imported.has_value());
    QCOMPARE(imported->name, QStringLiteral("Drums.v2"));
}

void TestProgramFile::test_notAProgramIsRefused()
{
    // 66 values, but not Akai's: position 2 should be 71.
    QByteArray content = readAll(fixture);
    content.replace("240 71 ", "240 72 ");
    const QString path = dir->filePath("other.lpd8");
    writeFile(path, content);

    const auto imported = importProgramFile(path);
    QVERIFY(!imported.has_value());
    QCOMPARE(imported.error(),
             QStringLiteral("%1 is not an LPD8 program.").arg(QDir::toNativeSeparators(path)));
}

void TestProgramFile::test_valueOutOfRangeIsRefused()
{
    // Channel 16 (the 9th value) is out of range: the message gives the
    // position a person counts in the file.
    QByteArray content = readAll(fixture);
    content.replace("99 0 1 1 3 ", "99 0 1 1 16 ");
    const QString path = dir->filePath("bad.lpd8");
    writeFile(path, content);

    const auto imported = importProgramFile(path);
    QVERIFY(!imported.has_value());
    QVERIFY(imported.error().startsWith(QDir::toNativeSeparators(path)));
    QVERIFY(imported.error().contains(QStringLiteral("position 9")));
}

void TestProgramFile::test_exportFailureIsReported()
{
    const QString path = dir->filePath("no/such/folder/p.lpd8");
    const auto written = exportProgramFile(fixtureProgram(), path);
    QVERIFY(!written.has_value());
    QVERIFY(written.error().contains(QDir::toNativeSeparators(path)));
    QVERIFY(!QFile::exists(path));
}

QTEST_GUILESS_MAIN(TestProgramFile)
#include "programfile_test.moc"
