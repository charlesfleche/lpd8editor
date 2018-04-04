#include "commands.h"
#include "db.h"
#include "programsmodel.h"

#include <QStandardItemModel>

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

private:
    QSqlDatabase db;
    ProgramsModel* pm;
};

void TestCommands::initTestCase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestCommands::init() {
    db.open();
    initDb(db);
    pm = new ProgramsModel(nullptr, db);
}

void TestCommands::cleanup() {
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

    createProgram();
    pm->select();
    const int r = 0;
    const int c = 2; // MIDI channel
    pm->setData(pm->index(r, c), 0, Qt::EditRole);

    UpdateParameterCommand cmd(pm, r, c, 7, Qt::EditRole);
    cmd.redo();
    QCOMPARE(pm->data(pm->index(r, c)).toInt(), 7);
    cmd.undo();
    QCOMPARE(pm->data(pm->index(r, c)).toInt(), 0);
}

QTEST_APPLESS_MAIN(TestCommands)

#include "tst_commands.moc"
