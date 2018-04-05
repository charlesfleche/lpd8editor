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
    void test_UpdateProgramFromSysexCommand();

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
    const QString initial("initial");
    const QString updated("updated");

    QStandardItemModel m;
    m.appendRow(new QStandardItem(initial));

    UpdateParameterCommand cmd(&m, 0, 0, updated, Qt::DisplayRole);
    cmd.redo();
    QCOMPARE(m.data(m.index(0, 0)).toString(), updated);
    cmd.undo();
    QCOMPARE(m.data(m.index(0, 0)).toString(), initial);
}

void TestCommands::test_UpdateProgramFromSysexCommand()
{
    const int id = createProgram();
    const QByteArray initial = programSysex(id);
    QByteArray updated(initial);
    updated[21] = (char)0;
    QVERIFY(initial != updated);
    UpdateProgramFromSysexCommand cmd(pm, id, updated);
    cmd.redo();
    QCOMPARE(programSysex(id), updated);
    cmd.undo();
    QCOMPARE(programSysex(id), initial);
}

QTEST_APPLESS_MAIN(TestCommands)

#include "tst_commands.moc"
