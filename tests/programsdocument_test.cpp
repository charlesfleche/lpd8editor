#include "deviceprofile.h"
#include "programsdocument.h"
#include "storecommands.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace lpd8;

namespace {

Program withChannel(quint8 channel)
{
    Program p = profile().defaultProgram;
    p.channel = channel;
    return p;
}

void pushChannel(ProgramsDocument &document, int id, quint8 channel)
{
    document.undoStack().push(
            makeSetProgramCommand(document.store(), id, withChannel(channel)).value().release());
}

void blockFolder(const QString &folder)
{
    // A regular file where the folder should be: saving there fails, even
    // for root.
    QDir(folder).removeRecursively();
    QFile f(folder);
    QVERIFY(f.open(QIODevice::WriteOnly));
}

} // namespace

class TestProgramsDocument : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_loadClearsTheUndoStack();
    void test_loadDoesNotWrite();
    void test_editsInOneTurnMakeOneWrite();
    void test_undoAndRedoSave();
    void test_directStoreChangesSave();
    void test_failureIsReportedOnceUntilASuccess();
    void test_saveFinalReturnsTheError();
    void test_saveFinalWithoutChangesDoesNothing();
    void test_destroyingWithAPendingSaveDoesNotWrite();
    void test_destroyingWithCommandsIsSafe();

private:
    std::unique_ptr<QTemporaryDir> dir;
    QString folder; // where the programs file goes
    QString path;
};

void TestProgramsDocument::init()
{
    dir = std::make_unique<QTemporaryDir>();
    folder = dir->filePath("data");
    path = folder + "/programs.json";
}

void TestProgramsDocument::test_loadClearsTheUndoStack()
{
    // W2: after a load, an id held by a command may name another program, so
    // the stack is emptied and undo does nothing.
    ProgramsDocument document(path);
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
    QVERIFY(document.saveFinal().has_value());
    pushChannel(document, id, 3);
    QCOMPARE(document.undoStack().count(), 1);

    QVERIFY(document.store().load().has_value());
    QCOMPARE(document.undoStack().count(), 0);
    document.undoStack().undo();
    // The loaded program has a new id: the old one names nothing.
    QVERIFY(!document.store().find(id).has_value());
    QCOMPARE(document.store().programs().size(), 1);
    QCOMPARE(document.store().programs().at(0).program, profile().defaultProgram);
}

void TestProgramsDocument::test_loadDoesNotWrite()
{
    ProgramsDocument document(path);
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
    QVERIFY(document.saveFinal().has_value());
    pushChannel(document, id, 3); // something to clear
    QCoreApplication::processEvents();
    const QSignalSpy saved(&document, &ProgramsDocument::saved);

    QVERIFY(document.store().load().has_value()); // clears the stack
    QCoreApplication::processEvents();
    QCOMPARE(saved.count(), 0);
}

void TestProgramsDocument::test_editsInOneTurnMakeOneWrite()
{
    ProgramsDocument document(path);
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
    const QSignalSpy saved(&document, &ProgramsDocument::saved);

    for (quint8 channel = 1; channel <= 5; ++channel) {
        pushChannel(document, id, channel);
    }
    QCOMPARE(saved.count(), 0); // not yet: at the next turn of the event loop
    QCoreApplication::processEvents();
    QCOMPARE(saved.count(), 1);

    ProgramStore written(path);
    QVERIFY(written.load().has_value());
    QCOMPARE(written.find(id).value().program.channel, quint8(5));
}

void TestProgramsDocument::test_undoAndRedoSave()
{
    ProgramsDocument document(path);
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
    pushChannel(document, id, 3);
    QCoreApplication::processEvents();
    const QSignalSpy saved(&document, &ProgramsDocument::saved);

    document.undoStack().undo();
    QCoreApplication::processEvents();
    document.undoStack().redo();
    QCoreApplication::processEvents();
    QCOMPARE(saved.count(), 2);
}

void TestProgramsDocument::test_directStoreChangesSave()
{
    // A change made without the undo stack is saved too: the document
    // follows the store, not the stack.
    ProgramsDocument document(path);
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
    QCoreApplication::processEvents();
    const QSignalSpy saved(&document, &ProgramsDocument::saved);

    QVERIFY(document.store().setName(id, QStringLiteral("Renamed directly")));
    QCoreApplication::processEvents();
    QCOMPARE(saved.count(), 1);
    ProgramStore written(path);
    QVERIFY(written.load().has_value());
    QCOMPARE(written.find(id).value().name, QStringLiteral("Renamed directly"));

    // A no-op changes nothing, so writes nothing.
    QVERIFY(document.store().setName(id, QStringLiteral("Renamed directly")));
    QCoreApplication::processEvents();
    QCOMPARE(saved.count(), 1);
}

void TestProgramsDocument::test_failureIsReportedOnceUntilASuccess()
{
    ProgramsDocument document(path);
    QStringList reports;
    document.setSaveFailureHandler([&reports](const QString &error) { reports << error; });
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();

    blockFolder(folder);
    for (quint8 channel = 1; channel <= 3; ++channel) { // three turns, three failed saves
        pushChannel(document, id, channel);
        QCoreApplication::processEvents();
    }
    QCOMPARE(reports.size(), 1); // reported once
    QVERIFY2(reports.at(0).startsWith("Cannot"), qPrintable(reports.at(0)));

    QVERIFY(QFile::remove(folder)); // the problem goes away: the next save works
    pushChannel(document, id, 4);
    QCoreApplication::processEvents();
    QCOMPARE(reports.size(), 1);

    blockFolder(folder); // and comes back: reported again
    pushChannel(document, id, 5);
    QCoreApplication::processEvents();
    QCOMPARE(reports.size(), 2);
}

void TestProgramsDocument::test_saveFinalReturnsTheError()
{
    ProgramsDocument document(path);
    int reports = 0;
    document.setSaveFailureHandler([&reports](const QString &) { ++reports; });
    const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
    blockFolder(folder);
    pushChannel(document, id, 1);
    QCoreApplication::processEvents();
    QCOMPARE(reports, 1); // the timer's save

    // The change isn't saved: saveFinal() tries again, and returns the
    // error to its caller instead of calling the handler. What can be
    // shown depends on the moment (closing, quitting), which only the
    // caller knows.
    const auto failed = document.saveFinal();
    QVERIFY(!failed.has_value());
    QVERIFY2(failed.error().startsWith("Cannot"), qPrintable(failed.error()));
    QCOMPARE(reports, 1);

    // Still not saved, so a second final save (closing, then quitting)
    // tries again: the changes are not given up on.
    QVERIFY(!document.saveFinal().has_value());
    QCOMPARE(reports, 1);

    // And once the disk is back, the change is written.
    QVERIFY(QFile::remove(folder));
    QVERIFY(document.saveFinal().has_value());
    ProgramStore written(path);
    QVERIFY(written.load().has_value());
    QCOMPARE(written.find(id).value().program.channel, 1);
}

void TestProgramsDocument::test_saveFinalWithoutChangesDoesNothing()
{
    // Quitting without an edit writes nothing: the file is created by the
    // first change, not by starting and quitting.
    ProgramsDocument document(path);
    QVERIFY(document.store().load().has_value());
    const QSignalSpy saved(&document, &ProgramsDocument::saved);
    QVERIFY(document.saveFinal().has_value());
    QCOMPARE(saved.count(), 0);
    QVERIFY(!QFile::exists(path));

    // After a save, the same.
    QVERIFY(document.store().add(QStringLiteral("A"), profile().defaultProgram).has_value());
    QVERIFY(document.saveFinal().has_value());
    QCOMPARE(saved.count(), 1);
    QVERIFY(document.saveFinal().has_value());
    QCOMPARE(saved.count(), 1);
}

void TestProgramsDocument::test_destroyingWithAPendingSaveDoesNotWrite()
{
    {
        ProgramsDocument document(path);
        const int id = document.store().add(QStringLiteral("A"), profile().defaultProgram).value();
        pushChannel(document, id, 3); // a save is now scheduled...
    } // ...and the document is gone before the event loop runs it
    QCoreApplication::processEvents();
    QVERIFY(!QFile::exists(path));
}

void TestProgramsDocument::test_destroyingWithCommandsIsSafe()
{
    // W3: the stack is destroyed before the store its commands refer to.
    // Under ASan, a wrong order would be reported here.
    auto document = std::make_unique<ProgramsDocument>(path);
    const int id = document->store().add(QStringLiteral("A"), profile().defaultProgram).value();
    pushChannel(*document, id, 3);
    document->undoStack().push(makeRemoveCommand(document->store(), id).value().release());
    document.reset();
}

QTEST_GUILESS_MAIN(TestProgramsDocument)

#include "programsdocument_test.moc"
