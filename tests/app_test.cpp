#include "app.h"
#include "deviceprofile.h"
#include "faketransport.h"
#include "lpd8device.h"
#include "mainwindow.h"
#include "miditransport.h"
#include "migration.h"
#include "programsdocument.h"
#include "programstore.h"
#include "sqlite_schema.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <functional>
#include <optional>

using namespace lpd8;

namespace {

using Answer = AppDialogs::Answer;

// The bytes of the file at 'path', or nothing if there is no regular file
// there.
std::optional<QByteArray> contentOf(const QString &path)
{
    QFile f(path);
    if (!QFileInfo(path).isFile() || !f.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return f.readAll();
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
        ok = db.open() && createLastSqliteSchema(db);
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

// The startup's dialogs, answered by the test. Records what was asked and
// told, in order.
class FakeDialogs : public AppDialogs
{
public:
    struct Question
    {
        QString text;
        QList<Answer> offered;
    };
    struct Notice
    {
        QWidget *parent{nullptr};
        QString text;
    };

    Answer ask(const QString &text, const QList<Answer> &offered) override
    {
        questions.append(Question{.text = text, .offered = offered});
        return answer(questions.last());
    }

    void tell(QWidget *parent, const QString &text) override
    {
        notices.append(Notice{.parent = parent, .text = text});
    }

    bool confirmCloseWithoutSaving(QWidget *parent, const QString &text) override
    {
        closeQuestions.append(Notice{.parent = parent, .text = text});
        return closeWithoutSaving;
    }

    QList<Question> questions;
    QList<Notice> notices;
    QList<Notice> closeQuestions;
    // The answer to confirmCloseWithoutSaving(): Cancel, unless the test
    // says otherwise.
    bool closeWithoutSaving{false};
    // Quit, unless the test says otherwise.
    std::function<Answer(const Question &)> answer = [](const Question &) { return Answer::Quit; };
};

// The offscreen platform plugin, which the tests run on, warns each time a
// window is shown. Only that exact message is ignored: every other warning
// fails the test (QTest::failOnWarning() in init()).
void expectWindowShown()
{
    if (QGuiApplication::platformName() == QLatin1StringView("offscreen")) {
        QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()");
    }
}

// The same, for a window whose tables are empty (no program): their first
// layout pass, in the event loop, resizes the window (the known quirk of
// doc/IMPROVEMENTS.md, 5.3), and the plugin warns again.
void expectEmptyWindowLaidOut()
{
    expectWindowShown();
}

// A migration that writes 'json' as the programs file and reports
// 'report': the way to make the load fail right after a migration.
App::Migrate migrationWriting(const QByteArray &json, const MigrationReport &report)
{
    return [json, report](const QString &, const QString &jsonPath)
                   -> std::expected<std::optional<MigrationReport>, MigrationError> {
        // Not writeFile(): its QVERIFY returns void, and this lambda can't.
        QFile f(jsonPath);
        if (!f.open(QIODevice::WriteOnly) || f.write(json) != json.size()) {
            return std::unexpected(MigrationError{.kind = MigrationError::Kind::Other,
                                                  .message = QStringLiteral("cannot write"),
                                                  .journalPath = {}});
        }
        return report;
    };
}

MigrationReport someReport(const QString &sqlitePath)
{
    return MigrationReport{.migrated = 1,
                           .skipped = {QStringLiteral("program 4 (Bass): channel is 20")},
                           .oldFileNowAt = sqlitePath + ".migrated",
                           .renameError = {},
                           .reportFileAt = sqlitePath + ".migrated.txt",
                           .reportFileError = {}};
}

// A session manager ending the session, for App::commitData().
class FakeSessionEnd : public SessionEnd
{
public:
    bool allowsInteraction() override
    {
        ++interactionRequests;
        return interaction;
    }
    void release() override { ++releases; }

    bool interaction{false};
    int interactionRequests{0};
    int releases{0};
};

// What a key press on the modal box saw.
struct ModalBoxSeen
{
    bool found{false};
    QString defaultButton;
    bool escapeIsDefault{false};
};

// Presses 'key' on the modal message box once it is shown, from inside its
// exec(): checks every 20 ms, up to a second. If no box comes, it closes
// every window, so exec() returns and the test fails on 'found' instead of
// hanging until its time limit.
void pressOnModalBox(QObject *context, Qt::Key key, ModalBoxSeen *seen, int triesLeft = 50)
{
    QTimer::singleShot(triesLeft == 50 ? 0 : 20, context, [context, key, seen, triesLeft] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (box == nullptr) {
            if (triesLeft > 0) {
                pressOnModalBox(context, key, seen, triesLeft - 1);
            } else {
                QApplication::closeAllWindows();
            }
            return;
        }
        seen->found = true;
        if (box->defaultButton() != nullptr) {
            seen->defaultButton = box->defaultButton()->text();
        }
        seen->escapeIsDefault = box->escapeButton() == box->defaultButton();
        QTest::keyClick(box, key);
    });
}

const QDateTime set_aside_time(QDate(2026, 1, 2), QTime(3, 4, 5));
constexpr const char *set_aside_suffix = ".corrupt-20260102-030405";

} // namespace

class TestApp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void test_firstStart();
    void test_migrationIsShownOverTheWindow();
    void test_migrationFailureQuit();
    void test_migrationFailureRetry();
    void test_hotJournalSaysWhereTheJournalIs();
    void test_damagedFileQuit();
    void test_damagedFileSetAside();
    void test_newerFormatCannotBeSetAside();
    void test_directoryCannotBeRead();
    void test_failedSetAside();
    void test_loadFailureAfterMigration();
    void test_damagedAfterMigrationSetAside();
    void test_aboutToQuitSaves();
    void test_commitDataSaves();
    void test_saveAndMidiProblemsShareTheWindowsBox();
    void test_failedCloseCancelKeepsTheChanges();
    void test_failedCloseWithoutSavingDoesNotAskAgain();
    void test_failedQuitShowsOneMessage();
    void test_failedCommitDataWithoutInteractionIsLogged();
    void test_silentSessionEndAsksNothingMore();
    void test_failedCommitDataWithInteractionIsShown();
    void test_closeBoxDefaultsToCancel_data();
    void test_closeBoxDefaultsToCancel();
    void test_messageBoxDefaultsToQuit_data();
    void test_messageBoxDefaultsToQuit();

private:
    App::Options options();
    // Starts a first start (an empty, writable store), adds one program
    // (its save pending), and makes every save fail from then on.
    void startWithFailingSaves(App &app);
    // Both files, as they are now.
    std::pair<std::optional<QByteArray>, std::optional<QByteArray>> files() const;

    QTemporaryDir configDir;
    std::unique_ptr<QTemporaryDir> dir;
    QString sqlitePath;
    QString jsonPath;
    FakeDialogs dialogs;
    int transportsMade{0};
};

void TestApp::initTestCase()
{
    // QSettings must never touch the real ~/.config.
    QVERIFY(configDir.isValid());
    qputenv("XDG_CONFIG_HOME", configDir.path().toLocal8Bit());
}

void TestApp::init()
{
    QTest::failOnWarning();
    dir = std::make_unique<QTemporaryDir>();
    QVERIFY(dir->isValid());
    sqlitePath = dir->filePath("db.sqlite3");
    jsonPath = dir->filePath("programs.json");
    dialogs.questions.clear();
    dialogs.notices.clear();
    dialogs.closeQuestions.clear();
    dialogs.closeWithoutSaving = false;
    dialogs.answer = [](const FakeDialogs::Question &) { return Answer::Quit; };
    transportsMade = 0;
}

App::Options TestApp::options()
{
    return App::Options{.sqlitePath = sqlitePath,
                        .storePath = jsonPath,
                        .dialogs = &dialogs,
                        // Counts, and turns MIDI off.
                        .makeTransport = [this]() -> std::unique_ptr<MidiTransport> {
                            ++transportsMade;
                            return nullptr;
                        },
                        .now = [] { return set_aside_time; },
                        .migrate = migrateSqliteDatabase};
}

std::pair<std::optional<QByteArray>, std::optional<QByteArray>> TestApp::files() const
{
    return {contentOf(sqlitePath), contentOf(jsonPath)};
}

void TestApp::test_firstStart()
{
    expectWindowShown();
    {
        App app(options());
        QVERIFY(app.start());
        QCOMPARE(transportsMade, 1);
        QVERIFY(app.window() != nullptr);
        QVERIFY(app.window()->isVisible());
        QVERIFY(app.document().store().programs().isEmpty());
        QVERIFY(dialogs.questions.isEmpty());
        QVERIFY(dialogs.notices.isEmpty());
        // A save scheduled by the startup would happen now.
        expectEmptyWindowLaidOut();
        QCoreApplication::processEvents();
    }
    // The file is created by the first edit, not by starting.
    QVERIFY(!QFile::exists(jsonPath));
    QVERIFY(!QFile::exists(sqlitePath));
}

void TestApp::test_migrationIsShownOverTheWindow()
{
    QVERIFY(buildDatabase(sqlitePath));

    expectWindowShown();
    App app(options());
    QVERIFY(app.start());
    QVERIFY(dialogs.questions.isEmpty());
    QCOMPARE(dialogs.notices.size(), 1);
    // Over the window, which exists by then.
    QVERIFY(app.window() != nullptr);
    QCOMPARE(dialogs.notices.at(0).parent, app.window());
    const MigrationReport report{.migrated = 2,
                                 .skipped = {},
                                 .oldFileNowAt = sqlitePath + ".migrated",
                                 .renameError = {},
                                 .reportFileAt = sqlitePath + ".migrated.txt",
                                 .reportFileError = {}};
    QCOMPARE(dialogs.notices.at(0).text, describe(report));
    // It names the report file next to the backup.
    QVERIFY(dialogs.notices.at(0).text.contains(QDir::toNativeSeparators(report.reportFileAt)));

    QCOMPARE(app.document().store().programs().size(), 2);
    QCOMPARE(app.document().store().programs().at(0).id, 3);
    QCOMPARE(app.document().store().programs().at(1).id, 7);
    QCOMPARE(app.document().undoStack().count(), 0);
}

void TestApp::test_migrationFailureQuit()
{
    const QByteArray garbage("this is not an SQLite database, but it is in its place");
    writeFile(sqlitePath, garbage);
    const auto before = files();

    {
        App app(options());
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QVERIFY(app.window() == nullptr);
        QCOMPARE(dialogs.questions.size(), 1);
        const auto &question = dialogs.questions.at(0);
        // Quit or Retry, never "start without migrating".
        QCOMPARE(question.offered, QList<Answer>{Answer::Retry});
        QVERIFY(question.text.contains(QDir::toNativeSeparators(sqlitePath)));
        QVERIFY(question.text.contains(QStringLiteral("Nothing was changed")));
        QVERIFY(question.text.contains(QStringLiteral("close it, then choose Retry")));
        QCoreApplication::processEvents();
    }
    QCOMPARE(files(), before);
    QVERIFY(!QFile::exists(jsonPath));
}

void TestApp::test_migrationFailureRetry()
{
    writeFile(sqlitePath, "this is not an SQLite database, but it is in its place");
    // The user repairs the database (here: replaces it), then retries.
    dialogs.answer = [this](const FakeDialogs::Question &) {
        QFile::remove(sqlitePath);
        return buildDatabase(sqlitePath) ? Answer::Retry : Answer::Quit;
    };

    expectWindowShown();
    App app(options());
    QVERIFY(app.start());
    QCOMPARE(dialogs.questions.size(), 1);
    QCOMPARE(app.document().store().programs().size(), 2);
    QCOMPARE(dialogs.notices.size(), 1);
    QCOMPARE(dialogs.notices.at(0).parent, app.window());
    QVERIFY(dialogs.notices.at(0).text.contains(QStringLiteral("2 program(s) moved")));
}

void TestApp::test_hotJournalSaysWhereTheJournalIs()
{
    const QString original = dir->filePath("original.sqlite3");
    QVERIFY(buildDatabase(original));
    QVERIFY(copyWithHotJournal(original, sqlitePath));
    const auto before = files();
    const auto journal = contentOf(sqlitePath + "-journal");

    {
        App app(options());
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QCOMPARE(dialogs.questions.size(), 1);
        const auto &question = dialogs.questions.at(0);
        QCOMPARE(question.offered, QList<Answer>{Answer::Retry});
        QVERIFY(question.text.contains(QDir::toNativeSeparators(sqlitePath + "-journal")));
        QVERIFY(question.text.contains(QStringLiteral("sqlite3")));
    }
    QCOMPARE(files(), before);
    QCOMPARE(contentOf(sqlitePath + "-journal"), journal);
}

void TestApp::test_damagedFileQuit()
{
    writeFile(jsonPath, "{ broken");
    const auto before = files();

    {
        App app(options());
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QCOMPARE(dialogs.questions.size(), 1);
        const auto &question = dialogs.questions.at(0);
        QCOMPARE(question.offered, QList<Answer>{Answer::SetAside});
        QVERIFY(question.text.contains(QDir::toNativeSeparators(jsonPath)));
        QVERIFY(question.text.contains(QStringLiteral("Nothing was changed")));
        QVERIFY(question.text.contains(QStringLiteral("may be damaged")));
        QVERIFY(dialogs.notices.isEmpty());
        QCoreApplication::processEvents();
    }
    QCOMPARE(files(), before);
}

void TestApp::test_damagedFileSetAside()
{
    const QByteArray damaged = "{ broken";
    writeFile(jsonPath, damaged);
    const QString aside = jsonPath + set_aside_suffix;
    dialogs.answer = [](const FakeDialogs::Question &) { return Answer::SetAside; };

    expectWindowShown();
    {
        App app(options());
        QVERIFY(app.start());
        QCOMPARE(dialogs.questions.size(), 1);
        // Renamed, never deleted, and the user is told where it went.
        QCOMPARE(contentOf(aside), std::optional(damaged));
        QVERIFY(!QFile::exists(jsonPath));
        QCOMPARE(dialogs.notices.size(), 1);
        QCOMPARE(dialogs.notices.at(0).parent, nullptr);
        QVERIFY(dialogs.notices.at(0).text.contains(QDir::toNativeSeparators(aside)));
        // Started empty, and the store may save again.
        QVERIFY(app.document().store().programs().isEmpty());
        QVERIFY(app.document().store().isWritable());
        expectEmptyWindowLaidOut();
        QCoreApplication::processEvents();
    }
    QVERIFY(!QFile::exists(jsonPath)); // starting wrote nothing

    // The next start is a first start, and leaves the set-aside file alone.
    dialogs.questions.clear();
    dialogs.notices.clear();
    expectWindowShown();
    {
        App app(options());
        QVERIFY(app.start());
        QVERIFY(dialogs.questions.isEmpty());
        QVERIFY(dialogs.notices.isEmpty());
        expectEmptyWindowLaidOut();
        QCoreApplication::processEvents();
    }
    QCOMPARE(contentOf(aside), std::optional(damaged));
    QVERIFY(!QFile::exists(jsonPath));
}

void TestApp::test_newerFormatCannotBeSetAside()
{
    // A file from a newer version, and an old database that must not be
    // migrated again over it.
    writeFile(jsonPath, R"({"format": 99, "programs": []})");
    QVERIFY(buildDatabase(sqlitePath));
    const auto before = files();
    // Even a user who would set it aside isn't offered to.
    dialogs.answer = [](const FakeDialogs::Question &) { return Answer::SetAside; };

    {
        App app(options());
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QCOMPARE(dialogs.questions.size(), 1);
        const auto &question = dialogs.questions.at(0);
        QVERIFY(question.offered.isEmpty()); // Quit only
        QVERIFY(question.text.contains(QStringLiteral("newer version")));
        QVERIFY(question.text.contains(QStringLiteral("Nothing was changed")));
        QCoreApplication::processEvents();
    }
    QCOMPARE(files(), before);
    QVERIFY(!QFile::exists(sqlitePath + ".migrated"));
    QVERIFY(!QFile::exists(jsonPath + set_aside_suffix));
}

void TestApp::test_directoryCannotBeRead()
{
    QVERIFY(QDir().mkpath(jsonPath));
    dialogs.answer = [](const FakeDialogs::Question &) { return Answer::SetAside; };

    {
        App app(options());
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QCOMPARE(dialogs.questions.size(), 1);
        const auto &question = dialogs.questions.at(0);
        QVERIFY(question.offered.isEmpty()); // Quit only
        QVERIFY(question.text.contains(QStringLiteral("not a regular file")));
        QVERIFY(question.text.contains(QStringLiteral("Check that you can read")));
    }
    QVERIFY(QFileInfo(jsonPath).isDir());
    QVERIFY(!QFile::exists(jsonPath + set_aside_suffix));
}

void TestApp::test_failedSetAside()
{
    // The target name is taken: the rename fails, the error is shown, and
    // Quit is the only choice left.
    const QByteArray damaged = "{ broken";
    writeFile(jsonPath, damaged);
    const QString taken = jsonPath + set_aside_suffix;
    writeFile(taken, "an earlier one");
    const auto before = files();
    dialogs.answer = [](const FakeDialogs::Question &) { return Answer::SetAside; };

    {
        App app(options());
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QCOMPARE(dialogs.questions.size(), 2);
        QCOMPARE(dialogs.questions.at(0).offered, QList<Answer>{Answer::SetAside});
        const auto &error = dialogs.questions.at(1);
        QVERIFY(error.offered.isEmpty()); // Quit only
        QVERIFY(error.text.contains(QDir::toNativeSeparators(taken)));
        QVERIFY(dialogs.notices.isEmpty());
        QCoreApplication::processEvents();
    }
    QCOMPARE(files(), before);
    QCOMPARE(contentOf(taken), std::optional(QByteArray("an earlier one")));
}

void TestApp::test_loadFailureAfterMigration()
{
    // The migration ran (it wrote the file and moved the database), and
    // then the file can't be loaded. The refusal must not claim that nothing
    // changed, and must carry the report, which no later start shows.
    const QByteArray newer(R"({"format": 99, "programs": []})");
    const MigrationReport report = someReport(sqlitePath);
    App::Options opts = options();
    opts.migrate = migrationWriting(newer, report);

    {
        App app(std::move(opts));
        QVERIFY(!app.start());
        QCOMPARE(transportsMade, 0); // a refused start opens no MIDI client
        QCOMPARE(dialogs.questions.size(), 1);
        const QString &message = dialogs.questions.at(0).text;
        QVERIFY(!message.contains(QStringLiteral("Nothing was changed")));
        QVERIFY(message.contains(QStringLiteral("This file was just created from your old "
                                                "database:\n\n")
                                 + describe(report)));
        QVERIFY(message.contains(QStringLiteral("newer version")));
        QVERIFY(message.contains(QDir::toNativeSeparators(jsonPath)));
        // Shown in the refusal, not again in a notice.
        QVERIFY(dialogs.notices.isEmpty());
        QCoreApplication::processEvents();
    }
    QCOMPARE(contentOf(jsonPath), std::optional(newer));
}

void TestApp::test_damagedAfterMigrationSetAside()
{
    // The file the migration just wrote is damaged, and the user sets it
    // aside. The report was in the question: the only notice is the one
    // about the set-aside, not the report again over an empty window.
    const QByteArray damaged = "{ broken";
    App::Options opts = options();
    opts.migrate = migrationWriting(damaged, someReport(sqlitePath));
    dialogs.answer = [](const FakeDialogs::Question &) { return Answer::SetAside; };

    expectWindowShown();
    App app(std::move(opts));
    QVERIFY(app.start());
    QCOMPARE(dialogs.questions.size(), 1);
    QVERIFY(dialogs.questions.at(0).text.contains(describe(someReport(sqlitePath))));
    QCOMPARE(dialogs.notices.size(), 1);
    QCOMPARE(dialogs.notices.at(0).parent, nullptr);
    QVERIFY(dialogs.notices.at(0).text.contains(
            QDir::toNativeSeparators(jsonPath + set_aside_suffix)));
    QCOMPARE(contentOf(jsonPath + set_aside_suffix), std::optional(damaged));
}

void TestApp::test_aboutToQuitSaves()
{
    // The Quit action and closing the last window end in quit(), which
    // emits aboutToQuit. A pending save (the zero-millisecond timer) never
    // fires after it, so App must save there.
    expectWindowShown();
    App app(options());
    QVERIFY(app.start());
    QVERIFY(app.document()
                    .store()
                    .add(QStringLiteral("Edited"), profile().defaultProgram)
                    .has_value());
    QVERIFY(!QFile::exists(jsonPath)); // the timer hasn't fired

    // A signal can't be emitted from outside its class, but invokeMethod
    // can call it by name, which emits it.
    QVERIFY(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit"));
    QVERIFY(contentOf(jsonPath).value_or(QByteArray()).contains("Edited"));
}

void TestApp::test_commitDataSaves()
{
    // Logging out: the session manager may end the application without
    // quit(), so commitData() saves.
    expectWindowShown();
    App app(options());
    QVERIFY(app.start());
    QVERIFY(app.document()
                    .store()
                    .add(QStringLiteral("Edited"), profile().defaultProgram)
                    .has_value());
    QVERIFY(!QFile::exists(jsonPath));

    FakeSessionEnd session;
    app.commitData(session);
    QVERIFY(contentOf(jsonPath).value_or(QByteArray()).contains("Edited"));
}

void TestApp::startWithFailingSaves(App &app)
{
    expectWindowShown();
    QVERIFY(app.start());
    // A directory where the file goes: QSaveFile can't replace it, even for
    // root.
    QVERIFY(QDir().mkpath(jsonPath));
    QVERIFY(app.document().store().add(QStringLiteral("Edited"), profile().defaultProgram));
}

void TestApp::test_saveAndMidiProblemsShareTheWindowsBox()
{
    // Every runtime problem goes to the window's one non-modal box: a MIDI
    // failure, then a save failure while it is up, give one box with both.
    App::Options opts = options();
    opts.makeTransport = []() -> std::unique_ptr<MidiTransport> {
        return std::make_unique<FakeTransport>();
    };
    App app(std::move(opts));
    startWithFailingSaves(app);
    QVERIFY(app.device() != nullptr);

    expectWindowShown(); // the box, once
    // Nothing is connected: the request fails at once.
    app.device()->requestProgram(1);
    expectEmptyWindowLaidOut();
    // The save's problem makes the box's text longer, which resizes it:
    // the plugin reports that like a show.
    expectWindowShown();
    QCoreApplication::processEvents(); // the save timer: it fails
    const QList<QMessageBox *> boxes = app.window()->findChildren<QMessageBox *>();
    QCOMPARE(boxes.size(), 1);
    QVERIFY(boxes.at(0)->isVisible());
    QCOMPARE(boxes.at(0)->windowModality(), Qt::NonModal);
    const QString text = boxes.at(0)->text();
    QVERIFY2(text.contains(QStringLiteral("connected")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("not saved yet")), qPrintable(text));
    // Neither was modal.
    QVERIFY(dialogs.notices.isEmpty());
}

void TestApp::test_failedCloseCancelKeepsTheChanges()
{
    App app(options());
    startWithFailingSaves(app);

    // Cancel: the window stays open, the change stays in memory.
    QVERIFY(!app.window()->close());
    QVERIFY(app.window()->isVisible());
    QCOMPARE(dialogs.closeQuestions.size(), 1);
    QCOMPARE(dialogs.closeQuestions.at(0).parent, app.window());
    QVERIFY(dialogs.closeQuestions.at(0).text.contains(QStringLiteral("could not be saved")));
    QCOMPARE(app.document().store().programs().size(), 1);

    // The problem goes away: the next close saves the change, without
    // asking.
    QVERIFY(QDir(jsonPath).removeRecursively());
    QVERIFY(app.window()->close());
    QCOMPARE(dialogs.closeQuestions.size(), 1);
    QVERIFY(contentOf(jsonPath).value_or(QByteArray()).contains("Edited"));
}

void TestApp::test_failedCloseWithoutSavingDoesNotAskAgain()
{
    App app(options());
    startWithFailingSaves(app);
    dialogs.closeWithoutSaving = true;

    QVERIFY(app.window()->close());
    QCOMPARE(dialogs.closeQuestions.size(), 1);
    // The last window closed, so the application quits next: the user has
    // chosen already, so nothing more is asked or shown.
    QVERIFY(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit"));
    QCOMPARE(dialogs.closeQuestions.size(), 1);
    QVERIFY(dialogs.notices.isEmpty());
}

void TestApp::test_failedQuitShowsOneMessage()
{
    // At aboutToQuit, the application ends whatever the user says: one
    // modal message, not a question, and no parent window.
    App app(options());
    startWithFailingSaves(app);

    QVERIFY(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit"));
    QCOMPARE(dialogs.notices.size(), 1);
    QCOMPARE(dialogs.notices.at(0).parent, nullptr);
    QVERIFY(dialogs.notices.at(0).text.contains(QStringLiteral("are lost")));
    QVERIFY(dialogs.closeQuestions.isEmpty());
}

void TestApp::test_failedCommitDataWithoutInteractionIsLogged()
{
    // The session manager forbids asking: the failure is logged, nothing is
    // shown, and the session isn't held.
    App app(options());
    startWithFailingSaves(app);

    FakeSessionEnd session;
    session.interaction = false;
    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Cannot save the programs at the end of the session"));
    app.commitData(session);
    QCOMPARE(session.interactionRequests, 1);
    QCOMPARE(session.releases, 0);
    QVERIFY(dialogs.notices.isEmpty());
}

void TestApp::test_silentSessionEndAsksNothingMore()
{
    // Without interaction, Qt turns the session's end into a quit, which
    // closes the window. After commitData() failed silently, neither the
    // close nor the quit may ask or show anything: a modal box would stall
    // the logout. Each failed retry is logged.
    App app(options());
    startWithFailingSaves(app);
    FakeSessionEnd session;
    session.interaction = false;
    const QRegularExpression logged("Cannot save the programs at the end of the session");
    QTest::ignoreMessage(QtCriticalMsg, logged);
    app.commitData(session);

    QTest::ignoreMessage(QtCriticalMsg, logged);
    QVERIFY(app.window()->close());
    QTest::ignoreMessage(QtCriticalMsg, logged);
    QVERIFY(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit"));
    QVERIFY(dialogs.closeQuestions.isEmpty());
    QVERIFY(dialogs.notices.isEmpty());
}

void TestApp::test_failedCommitDataWithInteractionIsShown()
{
    // Allowed to ask: shown (modally), then the session is released.
    App app(options());
    startWithFailingSaves(app);

    FakeSessionEnd session;
    session.interaction = true;
    app.commitData(session);
    QCOMPARE(session.interactionRequests, 1);
    QCOMPARE(session.releases, 1);
    QCOMPARE(dialogs.notices.size(), 1);
    QVERIFY(dialogs.notices.at(0).text.contains(QStringLiteral("could not be saved")));
}

void TestApp::test_messageBoxDefaultsToQuit_data()
{
    QTest::addColumn<int>("key");
    QTest::newRow("Escape") << int{Qt::Key_Escape};
    QTest::newRow("Return") << int{Qt::Key_Return};
}

void TestApp::test_messageBoxDefaultsToQuit()
{
    // The real dialogs, not the fake: a user who presses Enter or Escape
    // without reading must get Quit, never Retry or Set Aside.
    QFETCH(const int, key);

    ModalBoxSeen seen;
    pressOnModalBox(this, static_cast<Qt::Key>(key), &seen);

    expectWindowShown(); // the message box
    MessageBoxDialogs real;
    const Answer answer = real.ask(QStringLiteral("A question"), {Answer::Retry, Answer::SetAside});
    QCOMPARE(answer, Answer::Quit);
    QVERIFY(seen.found);
    QCOMPARE(seen.defaultButton, QStringLiteral("Quit"));
    QVERIFY(seen.escapeIsDefault);
}

void TestApp::test_closeBoxDefaultsToCancel_data()
{
    test_messageBoxDefaultsToQuit_data();
}

void TestApp::test_closeBoxDefaultsToCancel()
{
    // The real close question: Enter or Escape keep the window and the
    // changes. Losing them takes a deliberate click.
    QFETCH(const int, key);

    ModalBoxSeen seen;
    pressOnModalBox(this, static_cast<Qt::Key>(key), &seen);

    expectWindowShown(); // the message box
    MessageBoxDialogs real;
    QVERIFY(!real.confirmCloseWithoutSaving(nullptr, QStringLiteral("Lose them?")));
    QVERIFY(seen.found);
    QCOMPARE(seen.defaultButton, QStringLiteral("Cancel"));
    QVERIFY(seen.escapeIsDefault);
}

QTEST_MAIN(TestApp)
#include "app_test.moc"
