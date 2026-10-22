#include "commands.h"
#include "db.h"
#include "programsmodel.h"

#include <QUndoStack>

#include <QtTest>

class TestCommands : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_CreateProgramCommand();
    void test_DeleteProgramCommand();
    void test_UpdateParameterCommand();
    void test_UpdateProgramFromSysexCommand();
    void test_undoEditAfterProgramWasDeletedAndRestored();

private:
    QSqlDatabase db;
    ProgramsModel *pm{};
};

void TestCommands::initTestCase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestCommands::init()
{
    db.open();
    initDb(db);
    pm = new ProgramsModel(nullptr, db);
}

void TestCommands::cleanup()
{
    delete pm;
    db.close();
}

void TestCommands::test_CreateProgramCommand()
{
    createProgram();
    createProgram();

    CreateProgramCommand cmd(pm, QString(), QByteArray());
    cmd.redo();
    QCOMPARE(programIds(), QList<int>() << 1 << 2 << 3);

    createProgram();

    cmd.undo();
    QCOMPARE(programIds(), QList<int>() << 1 << 2 << 4);
}

void TestCommands::test_DeleteProgramCommand()
{
    createProgram();
    createProgram();
    createProgram();

    DeleteProgramCommand cmd(pm, 2);
    cmd.redo();
    QCOMPARE(programIds(), QList<int>() << 1 << 3);

    createProgram();
    QCOMPARE(programIds(), QList<int>() << 1 << 3 << 4);

    cmd.undo();
    QCOMPARE(programIds(), QList<int>() << 1 << 2 << 3 << 4);
}

void TestCommands::test_UpdateParameterCommand()
{
    const int id = pm->createProgram(QStringLiteral("initial"));
    const int nameColumn = 1; // programId, name, channel
    const ParameterLocation name{.programId = id,
                                 .group = ParameterLocation::Group::Program,
                                 .controlId = 0,
                                 .column = nameColumn};

    UpdateParameterCommand cmd(pm, name, QStringLiteral("updated"));
    cmd.redo();
    QCOMPARE(pm->parameterIndex(name).data().toString(), QStringLiteral("updated"));
    cmd.undo();
    QCOMPARE(pm->parameterIndex(name).data().toString(), QStringLiteral("initial"));
}

void TestCommands::test_UpdateProgramFromSysexCommand()
{
    const int id = createProgram();
    const QByteArray initial = programSysex(id);
    QByteArray updated(initial);
    updated[21] = '\0';
    QVERIFY(initial != updated);
    UpdateProgramFromSysexCommand cmd(pm, id, updated);
    cmd.redo();
    QCOMPARE(programSysex(id), updated);
    cmd.undo();
    QCOMPARE(programSysex(id), initial);
}

void TestCommands::test_undoEditAfterProgramWasDeletedAndRestored()
{
    // Edit a pad, delete its program, then undo both. Undoing the deletion
    // recreates the program with new internal models, and the edit's undo
    // must still find the pad. This used to be a heap-use-after-free: the
    // edit's command kept a pointer to a model deleted with the program.
    const int id = pm->createProgram();
    const QModelIndex pads = pm->padsParentIndex(pm->programIndex(id));
    const int noteColumn = 2; // programId, controlId, note, ...
    QVERIFY(pm->setData(pm->index(0, noteColumn, pads), 60, Qt::EditRole));

    QUndoStack *stack = undoStack();
    stack->push(new DeleteProgramCommand(pm, id));
    // Run the deferred deletions (deleteLater()), as the event loop would
    // between two user actions.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    stack->undo(); // restores the program, with the note at 60
    const QModelIndex restoredPads = pm->padsParentIndex(pm->programIndex(id));
    QCOMPARE(pm->index(0, noteColumn, restoredPads).data(Qt::EditRole).toInt(), 60);

    stack->undo(); // undoes the edit
    QCOMPARE(pm->index(0, noteColumn, restoredPads).data(Qt::EditRole).toInt(), 36);
}

QTEST_GUILESS_MAIN(TestCommands)

#include "commands_test.moc"
