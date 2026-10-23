#include "deviceprofile.h"
#include "programstore.h"
#include "storecommands.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

using namespace lpd8;

namespace {

Program withChannel(quint8 channel)
{
    Program p = profile().defaultProgram;
    p.channel = channel;
    return p;
}

QList<int> ids(const ProgramStore &store)
{
    QList<int> result;
    for (const StoredProgram &p : store.programs()) {
        result.append(p.id);
    }
    return result;
}

} // namespace

class TestStoreCommands : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_setProgram();
    void test_rename();
    void test_noOpCommandsAreNotMade();
    void test_refusalsSayWhy();
    void test_addUndoRedoKeepsIdAndRow();
    void test_removeRestoresTheRow();
    void test_commandForAGoneProgramBecomesObsolete();
    void test_undoRemoveAfterOutsideRemovalsGoesAtTheEnd();
    void test_editsDoNotMerge();

private:
    std::unique_ptr<QTemporaryDir> dir;
    std::unique_ptr<ProgramStore> store;
    QUndoStack stack;
};

void TestStoreCommands::init()
{
    stack.clear();
    dir = std::make_unique<QTemporaryDir>();
    store = std::make_unique<ProgramStore>(dir->filePath("programs.json"));
    for (const char *name : {"A", "B", "C", "D", "E"}) {
        store->add(QString::fromLatin1(name), profile().defaultProgram);
    }
}

void TestStoreCommands::test_setProgram()
{
    const int id = store->programs().at(1).id;
    stack.push(makeSetProgramCommand(*store, id, withChannel(3)).value().release());
    QCOMPARE(store->find(id).value().program.channel, quint8(3));
    stack.undo();
    QCOMPARE(store->find(id).value().program, profile().defaultProgram);
    stack.redo();
    QCOMPARE(store->find(id).value().program.channel, quint8(3));
}

void TestStoreCommands::test_rename()
{
    const int id = store->programs().at(0).id;
    stack.push(makeRenameCommand(*store, id, QStringLiteral("Drums")).value().release());
    QCOMPARE(store->find(id).value().name, QStringLiteral("Drums"));
    stack.undo();
    QCOMPARE(store->find(id).value().name, QStringLiteral("A"));
}

void TestStoreCommands::test_noOpCommandsAreNotMade()
{
    // The same value: no command (a value holding nullptr), so nothing is
    // pushed. A model's setData() returns true for it.
    const int id = store->programs().at(0).id;
    QCOMPARE(makeSetProgramCommand(*store, id, profile().defaultProgram).value(), nullptr);
    QCOMPARE(makeRenameCommand(*store, id, QStringLiteral("A")).value(), nullptr);
}

void TestStoreCommands::test_refusalsSayWhy()
{
    // Refusals are errors, told apart from no-ops: setData() returns false.
    const int id = store->programs().at(0).id;
    const auto refusal = [](const CommandOrRefusal &made) {
        return made.has_value() ? std::optional<CommandRefusal>() : made.error();
    };
    QCOMPARE(refusal(makeSetProgramCommand(*store, 999, withChannel(3))),
             std::optional(CommandRefusal::UnknownProgram));
    QCOMPARE(refusal(makeRenameCommand(*store, 999, QStringLiteral("X"))),
             std::optional(CommandRefusal::UnknownProgram));
    QCOMPARE(refusal(makeRemoveCommand(*store, 999)),
             std::optional(CommandRefusal::UnknownProgram));
    QCOMPARE(refusal(makeSetProgramCommand(*store, id, withChannel(16))),
             std::optional(CommandRefusal::InvalidValue));
    QCOMPARE(refusal(makeRenameCommand(*store, id, QString(QChar(0xd800)))),
             std::optional(CommandRefusal::InvalidValue));
    QCOMPARE(refusal(makeAddCommand(*store, QStringLiteral("X"), withChannel(16))),
             std::optional(CommandRefusal::InvalidValue));
}

void TestStoreCommands::test_addUndoRedoKeepsIdAndRow()
{
    stack.push(makeAddCommand(*store, QStringLiteral("F"), withChannel(4)).value().release());
    const int id = store->programs().last().id;
    const QList<int> after = ids(*store);

    for (int i = 0; i < 3; ++i) {
        stack.undo();
        QVERIFY(!store->find(id).has_value());
        stack.redo();
        QCOMPARE(ids(*store), after); // the same id, at the same row
        QCOMPARE(store->find(id).value().program.channel, quint8(4));
    }
}

void TestStoreCommands::test_removeRestoresTheRow()
{
    const QList<int> before = ids(*store);
    const int middle = before.at(2);
    stack.push(makeRemoveCommand(*store, middle).value().release());
    QVERIFY(!store->find(middle).has_value());
    QCOMPARE(store->programs().size(), 4);

    // Undo puts it back where it was, not at the end; redo and undo again
    // must not drift.
    for (int i = 0; i < 3; ++i) {
        stack.undo();
        QCOMPARE(ids(*store), before);
        QCOMPARE(store->find(middle).value().name, QStringLiteral("C"));
        stack.redo();
        QCOMPARE(store->programs().size(), 4);
    }
}

void TestStoreCommands::test_commandForAGoneProgramBecomesObsolete()
{
    // A command whose program was removed some other way does nothing and is
    // dropped by the stack, instead of acting on another program.
    const int id = store->programs().at(0).id;
    stack.push(makeSetProgramCommand(*store, id, withChannel(3)).value().release());
    QCOMPARE(stack.count(), 1);
    QVERIFY(store->remove(id)); // not through the stack

    stack.undo();
    QCOMPARE(stack.count(), 0); // obsolete, dropped
    QVERIFY(!store->find(id).has_value());
}

void TestStoreCommands::test_undoRemoveAfterOutsideRemovalsGoesAtTheEnd()
{
    // Remove the last program through the stack, then two others some other
    // way: its saved row is past the end, and undo puts it at the end rather
    // than losing it.
    const int last = store->programs().last().id;
    stack.push(makeRemoveCommand(*store, last).value().release());
    QVERIFY(store->remove(store->programs().at(0).id));
    QVERIFY(store->remove(store->programs().at(0).id));

    stack.undo();
    QCOMPARE(store->programs().last().id, last);
    QCOMPARE(stack.count(), 1); // not obsolete
}

void TestStoreCommands::test_editsDoNotMerge()
{
    // As the SQLite commands they replace: each edit is one undo step.
    const int id = store->programs().at(0).id;
    stack.push(makeSetProgramCommand(*store, id, withChannel(1)).value().release());
    stack.push(makeSetProgramCommand(*store, id, withChannel(2)).value().release());
    QCOMPARE(stack.count(), 2);
    stack.undo();
    QCOMPARE(store->find(id).value().program.channel, quint8(1));
}

QTEST_GUILESS_MAIN(TestStoreCommands)

#include "storecommands_test.moc"
