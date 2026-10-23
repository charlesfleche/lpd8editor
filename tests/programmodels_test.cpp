#include "deviceprofile.h"
#include "enums.h"
#include "programmodels.h"
#include "programstore.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

using namespace lpd8;

namespace {

using Tester = QAbstractItemModelTester;
constexpr auto fatal = Tester::FailureReportingMode::Fatal;

Program withChannel(quint8 channel)
{
    Program p = profile().defaultProgram;
    p.channel = channel;
    return p;
}

// The (top-left, bottom-right) of each dataChanged signal, as text.
QStringList ranges(const QSignalSpy &spy)
{
    QStringList result;
    for (const QList<QVariant> &args : spy) {
        const auto tl = args.at(0).value<QModelIndex>();
        const auto br = args.at(1).value<QModelIndex>();
        result << QStringLiteral("(%1,%2)-(%3,%4)")
                          .arg(QString::number(tl.row()),
                               QString::number(tl.column()),
                               QString::number(br.row()),
                               QString::number(br.column()));
    }
    return result;
}

} // namespace

class TestProgramModels : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_listFollowsTheStore();
    void test_listSignalsOnlyWhatChanged();
    void test_listSetData();
    void test_controlsShowAProgram();
    void test_controlsHeaders();
    void test_controlsFollowTheStore();
    void test_controlsSetData();
    void test_controlsSetDataRefusesWhatToIntWouldWrap();
    void test_toggleColumnIsNotEnabled();
    void test_resetEmptiesTheControls();

private:
    std::unique_ptr<QTemporaryDir> dir;
    std::unique_ptr<ProgramStore> store;
    std::unique_ptr<QUndoStack> stack;
};

void TestProgramModels::init()
{
    dir = std::make_unique<QTemporaryDir>();
    store = std::make_unique<ProgramStore>(dir->filePath("programs.json"));
    stack = std::make_unique<QUndoStack>();
    store->add(QStringLiteral("A"), profile().defaultProgram);
    store->add(QStringLiteral("B"), profile().defaultProgram);
}

void TestProgramModels::test_listFollowsTheStore()
{
    ProgramListModel model(*store, *stack);
    const Tester tester(&model, fatal);
    const QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    const QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), 2);

    // Changes made to the store directly, not through the model.
    const int c = store->insert(1, QStringLiteral("C"), profile().defaultProgram).value();
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(inserted.at(0).at(1).toInt(), 1);
    QCOMPARE(model.index(1, 0).data().toString(), QStringLiteral("C"));
    QCOMPARE(model.programId(1), c);

    QVERIFY(store->remove(store->programs().at(0).id));
    QCOMPARE(removed.count(), 1);
    QCOMPARE(removed.at(0).at(1).toInt(), 0);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.index(0, 0).data(ProgramListModel::IdRole).toInt(), c);
}

void TestProgramModels::test_listSignalsOnlyWhatChanged()
{
    ProgramListModel model(*store, *stack);
    const Tester tester(&model, fatal);
    const QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    const int b = store->programs().at(1).id;

    QVERIFY(store->setName(b, QStringLiteral("Drums"))); // one cell: the name
    QVERIFY(store->setProgram(b, withChannel(3))); // one cell: the channel
    QCOMPARE(ranges(changed), (QStringList{"(1,0)-(1,0)", "(1,1)-(1,1)"}));
    QCOMPARE(model.index(1, 1).data().toString(), QStringLiteral("4")); // channels count from 1
    QCOMPARE(model.index(1, 1).data(Qt::EditRole).toInt(), 3);
}

void TestProgramModels::test_listSetData()
{
    ProgramListModel model(*store, *stack);
    const Tester tester(&model, fatal);
    const QModelIndex name = model.index(0, 0);
    const QModelIndex channel = model.index(0, 1);

    QVERIFY(model.setData(name, QStringLiteral("Drums")));
    QCOMPARE(stack->count(), 1);
    QVERIFY(model.setData(name, QStringLiteral("Drums"))); // a no-op is accepted...
    QCOMPARE(stack->count(), 1); // ...and pushes nothing
    QVERIFY(!model.setData(name, QString(QChar(0xd800)))); // refused
    QVERIFY(model.setData(channel, 15));
    QVERIFY(!model.setData(channel, 16));
    QVERIFY(!model.setData(channel, QStringLiteral("3"))); // not an integer
    QCOMPARE(stack->count(), 2);

    stack->undo();
    stack->undo();
    QCOMPARE(store->programs().at(0).name, QStringLiteral("A"));
    QCOMPARE(store->programs().at(0).program.channel, quint8(9));
}

void TestProgramModels::test_controlsShowAProgram()
{
    ControlsModel pads(*store, *stack, ControlsModel::Controls::Pads);
    const Tester tester(&pads, fatal);
    QCOMPARE(pads.rowCount(), 0); // no program yet
    QCOMPARE(pads.columnCount(), 4);

    pads.setProgramId(store->programs().at(0).id);
    QCOMPARE(pads.rowCount(), 8);
    QCOMPARE(pads.index(0, ControlsModel::NoteColumn).data().toString(), QStringLiteral("36: C1"));
    QCOMPARE(pads.index(0, ControlsModel::NoteColumn).data(Qt::EditRole).toInt(), 36);
    QCOMPARE(pads.index(0, ControlsModel::ToggleColumn).data().toString(),
             QStringLiteral("Momentary"));
    QCOMPARE(pads.index(0, ControlsModel::NoteColumn)
                     .data(MidiDataRole::MidiValues)
                     .toStringList()
                     .size(),
             128);

    pads.setProgramId(999); // unknown: nothing shown
    QCOMPARE(pads.rowCount(), 0);

    ControlsModel knobs(*store, *stack, ControlsModel::Controls::Knobs);
    const Tester knobsTester(&knobs, fatal);
    knobs.setProgramId(store->programs().at(0).id);
    QCOMPARE(knobs.rowCount(), 8);
    QCOMPARE(knobs.columnCount(), 3);
    QCOMPARE(knobs.index(7, ControlsModel::HighColumn).data(Qt::EditRole).toInt(), 127);
}

void TestProgramModels::test_controlsHeaders()
{
    // Readable headers, not the SQL column names (pc, cc, toggle).
    const ControlsModel pads(*store, *stack, ControlsModel::Controls::Pads);
    const ControlsModel knobs(*store, *stack, ControlsModel::Controls::Knobs);
    const auto headers = [](const QAbstractItemModel &m) {
        QStringList result;
        for (int c = 0; c < m.columnCount(); ++c) {
            result << m.headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        }
        return result;
    };
    QCOMPARE(headers(pads), (QStringList{"Note", "Program change", "Control change", "Mode"}));
    QCOMPARE(headers(knobs), (QStringList{"Control change", "Low", "High"}));
}

void TestProgramModels::test_controlsFollowTheStore()
{
    const int a = store->programs().at(0).id;
    const int b = store->programs().at(1).id;
    ControlsModel pads(*store, *stack, ControlsModel::Controls::Pads);
    const Tester tester(&pads, fatal);
    pads.setProgramId(a);
    const QSignalSpy changed(&pads, &QAbstractItemModel::dataChanged);

    QVERIFY(store->setProgram(b, withChannel(1))); // another program: nothing
    QVERIFY(store->setName(a, QStringLiteral("X"))); // a rename: nothing here
    QVERIFY(changed.isEmpty());

    Program p = profile().defaultProgram;
    p.pads[2].note = 60;
    QVERIFY(store->setProgram(a, p)); // the shown program: all its control cells
    QCOMPARE(ranges(changed), QStringList{"(0,0)-(7,3)"});
    QCOMPARE(pads.index(2, ControlsModel::NoteColumn).data(Qt::EditRole).toInt(), 60);

    // The shown program is removed: nothing is shown.
    const QSignalSpy reset(&pads, &QAbstractItemModel::modelReset);
    QVERIFY(store->remove(a));
    QCOMPARE(reset.count(), 1);
    QCOMPARE(pads.rowCount(), 0);
    QCOMPARE(pads.programId(), -1);
}

void TestProgramModels::test_controlsSetData()
{
    const int a = store->programs().at(0).id;
    ControlsModel knobs(*store, *stack, ControlsModel::Controls::Knobs);
    const Tester tester(&knobs, fatal);
    knobs.setProgramId(a);
    const QModelIndex high = knobs.index(7, ControlsModel::HighColumn);

    QVERIFY(knobs.setData(high, 100));
    QCOMPARE(store->find(a).value().program.knobs[7].high, quint8(100));
    QCOMPARE(stack->count(), 1);
    QVERIFY(knobs.setData(high, 100)); // no-op: accepted, nothing pushed
    QCOMPARE(stack->count(), 1);
    QVERIFY(!knobs.setData(high, 128)); // out of range: refused
    QVERIFY(!knobs.setData(high, -1));
    QVERIFY(!knobs.setData(high, 100, Qt::DisplayRole));
    QCOMPARE(stack->count(), 1);

    stack->undo();
    QCOMPARE(store->find(a).value().program.knobs[7].high, quint8(127));
}

void TestProgramModels::test_controlsSetDataRefusesWhatToIntWouldWrap()
{
    // QVariant::toInt() turns these into 40, 37 and 1: they must be refused,
    // not stored as those values.
    ControlsModel pads(*store, *stack, ControlsModel::Controls::Pads);
    const Tester tester(&pads, fatal);
    pads.setProgramId(store->programs().at(0).id);
    const QModelIndex note = pads.index(0, ControlsModel::NoteColumn);
    QVERIFY(!pads.setData(note, QVariant(qlonglong(4294967336))));
    QVERIFY(!pads.setData(note, QVariant(36.5)));
    QVERIFY(!pads.setData(note, QVariant(QStringLiteral("abc"))));
    QVERIFY(!pads.setData(pads.index(0, ControlsModel::ToggleColumn),
                          QVariant(qlonglong(4294967297))));
    QCOMPARE(stack->count(), 0);
}

void TestProgramModels::test_toggleColumnIsNotEnabled()
{
    // As in the SQLite model (no behaviour change in this item).
    ControlsModel pads(*store, *stack, ControlsModel::Controls::Pads);
    pads.setProgramId(store->programs().at(0).id);
    QVERIFY(!pads.flags(pads.index(0, ControlsModel::ToggleColumn)).testFlag(Qt::ItemIsEnabled));
    QVERIFY(pads.flags(pads.index(0, ControlsModel::NoteColumn)).testFlag(Qt::ItemIsEnabled));
}

void TestProgramModels::test_resetEmptiesTheControls()
{
    QVERIFY(store->save().has_value());
    ProgramListModel list(*store, *stack);
    ControlsModel pads(*store, *stack, ControlsModel::Controls::Pads);
    const Tester listTester(&list, fatal);
    const Tester padsTester(&pads, fatal);
    pads.setProgramId(store->programs().at(0).id);
    const QSignalSpy listReset(&list, &QAbstractItemModel::modelReset);
    const QSignalSpy padsReset(&pads, &QAbstractItemModel::modelReset);

    QVERIFY(store->load().has_value());
    QCOMPARE(listReset.count(), 1);
    QCOMPARE(padsReset.count(), 1);
    QCOMPARE(list.rowCount(), 2);
    QCOMPARE(pads.rowCount(), 0); // the window picks a program again
}

QTEST_GUILESS_MAIN(TestProgramModels)

#include "programmodels_test.moc"
