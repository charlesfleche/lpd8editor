#include "deviceprofile.h"
#include "faketransport.h"
#include "lpd8codec.h"
#include "lpd8device.h"
#include "lpd8simulator.h"
#include "mainwindow.h"
#include "midiconnectionsmodel.h"
#include "programmodels.h"
#include "programsdocument.h"

#include <QAction>
#include <QItemSelectionModel>
#include <QListView>
#include <QMessageBox>
#include <QPointer>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QUndoStack>
#include <QtTest>

using namespace lpd8;

namespace {

// A window is about to be shown, by the test or by the window itself (its
// message box). The offscreen platform plugin, which the tests run on,
// warns that it can't propagate size hints each time a window is shown.
// That warning is the plugin's, not the window's, and only that exact
// message is ignored: every other warning fails the test
// (QTest::failOnWarning() in init()). It can't be filtered by category: it
// is in the 'default' category, with the application's own warnings.
void expectWindowShown()
{
    if (QGuiApplication::platformName() == QLatin1StringView("offscreen")) {
        QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()");
    }
}

void show(QWidget &window)
{
    expectWindowShown();
    window.show();
}

// A program dump as the device sends it: the storage format with the
// 'program dump' opcode (63 00 3a, doc/SYSEX.md).
QByteArray dumpOf(const Program &program)
{
    QByteArray sysex = encodeStoredProgram(program);
    sysex[6] = '\x3a';
    return sysex;
}

constexpr int wait_ms = 2000;

Program withChannel(quint8 channel)
{
    Program p = profile().defaultProgram;
    p.channel = channel;
    return p;
}

QAction *actionWithShortcut(const QWidget &window, QKeySequence::StandardKey key)
{
    const QKeySequence wanted(key);
    for (QAction *action : window.findChildren<QAction *>()) {
        if (action->shortcuts().contains(wanted)) {
            return action;
        }
    }
    return nullptr;
}

} // namespace

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_destroyWithSelectedProgram();
    void test_viewsHaveTheExactColumns();
    void test_editUndoRedoThroughThePadsView();
    void test_selectionFollowsRenamesAndRemovals();
    void test_getThroughTheSimulator();
    void test_sendThroughTheSimulator();
    void test_answerGoesToTheProgramSelectedAtRequest();
    void test_targetDeletedBeforeTheAnswer();
    void test_failureShowsOneNonModalMessage();
    void test_unsolicitedDumpIsIgnored();
    void test_midiActionsGoThroughTheTransport();
    void test_midiOff();
    void test_closeAsksTheGuard();
    void test_problemsShareOneBox();
    void test_problemsArePlainText();
    void test_newProgramUsesTheDefaultSettings();
    void test_newProgramIgnoresInvalidDefaultSetting();

private:
    // A window over 'document', with 'count' new programs and the last one
    // selected.
    std::unique_ptr<MainWindow> windowWithPrograms(int count);
    QListView *programsView(const MainWindow &window);

    QTemporaryDir configDir;
    std::unique_ptr<QTemporaryDir> dataDir;
    QString path;
    std::unique_ptr<ProgramsDocument> document;
    // The window's MIDI: a fake with a simulated LPD8 on "20:0", so no test
    // depends on the machine's sequencer. Declared after the document, reset
    // before it.
    std::unique_ptr<FakeTransport> transport;
    std::unique_ptr<Lpd8Simulator> simulator;
    // The device the window uses and doesn't own, as App gives it.
    std::unique_ptr<Lpd8Device> device;
};

void TestMainWindow::initTestCase()
{
    // QSettings must never touch the real ~/.config. (Qt's own test mode
    // would write to ~/.qttest instead, still in the home folder.)
    QVERIFY(configDir.isValid());
    qputenv("XDG_CONFIG_HOME", configDir.path().toLocal8Bit());
}

void TestMainWindow::init()
{
    QTest::failOnWarning();
    dataDir = std::make_unique<QTemporaryDir>();
    path = dataDir->filePath("programs.json");
    document = std::make_unique<ProgramsDocument>(path);
    QVERIFY(document->store().load().has_value());
    transport = std::make_unique<FakeTransport>();
    simulator = std::make_unique<Lpd8Simulator>(*transport); // "20:0"
    device = std::make_unique<Lpd8Device>(*transport);
}

void TestMainWindow::cleanup()
{
    // Each test destroys its window first: the transport and the document
    // outlive it.
    simulator.reset();
    device.reset();
    transport.reset();
    document.reset();
    QSettings().clear();
}

std::unique_ptr<MainWindow> TestMainWindow::windowWithPrograms(int count)
{
    auto window = std::make_unique<MainWindow>(*document, transport.get(), device.get());
    auto *newProgram = window->findChild<QAction *>("actionNewProgram");
    for (int i = 0; i < count; ++i) {
        newProgram->trigger();
    }
    QListView *view = programsView(*window);
    if (count > 0) {
        view->setCurrentIndex(view->model()->index(count - 1, view->modelColumn()));
    }
    return window;
}

QListView *TestMainWindow::programsView(const MainWindow &window)
{
    return window.findChild<QListView *>("programsView");
}

void TestMainWindow::test_destroyWithSelectedProgram()
{
    // Destroying the window runs ~MainWindow, which destroys 'ui', and then
    // the base class destructors, which delete the children (models, views,
    // MIDI objects). Signals emitted during that teardown must not reach
    // slots or lambdas that use 'ui'. Run under ASan to check.
    auto window = windowWithPrograms(2);
    QVERIFY(transport->connectTo("20:0"));
    show(*window);
    QListView *view = programsView(*window);
    QCOMPARE(view->model()->rowCount(), 2);
    QVERIFY(view->currentIndex().isValid());

    // Make signals fire during the teardown, through the window's own
    // connections, at two moments: when the programs model is destroyed,
    // and when the central widget is, in the children teardown, after 'ui'
    // is gone. Emit what the selection model, the MIDI model and the
    // transport can emit (Qt signals are public functions). Every slot or
    // lambda of the window they reach must already be disconnected, or ASan
    // reports the freed 'ui'. QPointer: each emitter may already be gone,
    // which is fine. The transport outlives the window, so it always emits.
    auto *programs = window->findChild<ProgramListModel *>();
    auto *central = window->findChild<QWidget *>("centralwidget");
    QVERIFY(programs && central);
    const QPointer<QItemSelectionModel> selection = view->selectionModel();
    const QPointer<MidiConnectionsModel> connections = window->findChild<MidiConnectionsModel *>();
    QVERIFY(connections);
    FakeTransport *midi = transport.get();
    Lpd8Device *dev = device.get();
    const QByteArray dump = dumpOf(withChannel(3));
    const auto fire = [selection, connections, midi, dev, dump] {
        if (selection) {
            emit selection->currentRowChanged({}, {});
            emit selection->selectionChanged({}, {});
        }
        if (connections) {
            emit connections->connectedChanged(true);
            emit connections->connectedPortChanged({});
        }
        emit midi->portsChanged();
        emit midi->connectionChanged();
        midi->receive(dump, "20:0");
        // The device outlives the window too (App owns it): ~MainWindow
        // must have disconnected it.
        emit dev->programReceived(1, withChannel(4));
        emit dev->sendDone(1);
        emit dev->requestFailed(1, QStringLiteral("gone"));
        emit dev->busyChanged(false);
    };
    connect(programs, &QObject::destroyed, this, fire);
    connect(central, &QObject::destroyed, this, fire);

    window.reset();
    // The store outlives the window, and changing it now reaches nothing.
    QVERIFY(document->store().add(QStringLiteral("After"), profile().defaultProgram));
}

void TestMainWindow::test_viewsHaveTheExactColumns()
{
    // No hidden id columns and no extra column: 4 fields for a pad, 3 for a
    // knob, with readable headers (not 'pc', 'cc' or 'toggle').
    const auto window = windowWithPrograms(1);
    auto *pads = window->findChild<QTableView *>("padsView");
    auto *knobs = window->findChild<QTableView *>("knobsView");
    QVERIFY(pads && knobs);
    QCOMPARE(pads->model()->columnCount(), 4);
    QCOMPARE(knobs->model()->columnCount(), 3);
    QCOMPARE(pads->model()->rowCount(), 8);
    QCOMPARE(knobs->model()->rowCount(), 8);
    QCOMPARE(pads->model()->headerData(1, Qt::Horizontal).toString(),
             QStringLiteral("Program change"));
    QCOMPARE(knobs->model()->headerData(0, Qt::Horizontal).toString(),
             QStringLiteral("Control change"));
}

void TestMainWindow::test_editUndoRedoThroughThePadsView()
{
    const auto window = windowWithPrograms(1);
    auto *pads = window->findChild<QTableView *>("padsView");
    QAction *undo = actionWithShortcut(*window, QKeySequence::Undo);
    QAction *redo = actionWithShortcut(*window, QKeySequence::Redo);
    QVERIFY(pads && undo && redo);

    const QModelIndex note = pads->model()->index(0, ControlsModel::NoteColumn);
    QCOMPARE(note.data().toString(), QStringLiteral("36: C1"));

    // An edit, as the delegate makes it.
    QVERIFY(pads->model()->setData(note, 60, Qt::EditRole));
    QCOMPARE(note.data().toString(), QStringLiteral("60: C3"));
    const int id = document->store().programs().at(0).id;
    QCOMPARE(document->store().get(id)->program.pads[0].note, 60);

    undo->trigger();
    QCOMPARE(note.data().toString(), QStringLiteral("36: C1"));
    QCOMPARE(document->store().get(id)->program, profile().defaultProgram);

    redo->trigger();
    QCOMPARE(note.data().toString(), QStringLiteral("60: C3"));
}

void TestMainWindow::test_selectionFollowsRenamesAndRemovals()
{
    const auto window = windowWithPrograms(3);
    QListView *view = programsView(*window);
    QAbstractItemModel *model = view->model();
    auto *deleteProgram = window->findChild<QAction *>("actionDeleteProgram");
    auto *stacked = window->findChild<QStackedWidget *>("stackedWidget");
    auto *pads = window->findChild<QTableView *>("padsView");
    const auto selectedId = [view] {
        return view->selectionModel()->isSelected(view->currentIndex())
                ? view->currentIndex().data(ProgramListModel::IdRole).toInt()
                : -1;
    };
    const auto shownId = [pads] {
        return qobject_cast<ControlsModel *>(pads->model())->programId();
    };
    const int a = document->store().programs().at(0).id;
    const int b = document->store().programs().at(1).id;
    const int c = document->store().programs().at(2).id;

    // Select B.
    view->setCurrentIndex(model->index(1, 0));
    QCOMPARE(selectedId(), b);
    QCOMPARE(shownId(), b);

    // A rename keeps it selected.
    QVERIFY(model->setData(model->index(0, 0), QStringLiteral("Renamed A")));
    QCOMPARE(selectedId(), b);

    // The removal of another program keeps it selected.
    QVERIFY(document->store().remove(a));
    QCOMPARE(selectedId(), b);
    QCOMPARE(shownId(), b);

    // Deleting the selected program selects its neighbour, and shows it.
    deleteProgram->trigger();
    QCOMPARE(selectedId(), c);
    QCOMPARE(shownId(), c);
    QVERIFY(deleteProgram->isEnabled());

    // Deleting the last one selects nothing, and shows the default page.
    deleteProgram->trigger();
    QCOMPARE(model->rowCount(), 0);
    QVERIFY(!view->currentIndex().isValid());
    QCOMPARE(shownId(), -1);
    QVERIFY(!deleteProgram->isEnabled());
    QCOMPARE(stacked->currentWidget()->objectName(), QStringLiteral("pageDefault"));
}

void TestMainWindow::test_getThroughTheSimulator()
{
    // End to end: Get program 2, the device answers, and the selected
    // program is now its slot 2, as one undo step, shown in the pads view.
    const auto window = windowWithPrograms(2);
    QVERIFY(transport->connectTo(simulator->portId()));
    Program inSlot2 = withChannel(5);
    inSlot2.pads[0].note = 60;
    simulator->setProgram(2, inSlot2);
    const int selected = document->store().programs().at(1).id;
    const int steps = document->undoStack().count();
    auto *get = window->findChild<QAction *>("actionGetProgram2");
    auto *pads = window->findChild<QTableView *>("padsView");

    get->trigger();
    QVERIFY(!get->isEnabled()); // one request at a time
    QVERIFY(QTest::qWaitFor([&] { return document->store().get(selected)->program == inSlot2; },
                            wait_ms));
    QCOMPARE(document->undoStack().count(), steps + 1);
    QCOMPARE(pads->model()->index(0, ControlsModel::NoteColumn).data().toString(),
             QStringLiteral("60: C3"));
    QCOMPARE(document->store().programs().at(0).program, profile().defaultProgram);
    QVERIFY(get->isEnabled());
}

void TestMainWindow::test_sendThroughTheSimulator()
{
    const auto window = windowWithPrograms(1);
    QVERIFY(transport->connectTo(simulator->portId()));
    auto *pads = window->findChild<QTableView *>("padsView");
    QVERIFY(pads->model()->setData(pads->model()->index(0, ControlsModel::NoteColumn), 48));
    const Program edited = document->store().programs().at(0).program;

    window->findChild<QAction *>("actionSendToProgram3")->trigger();
    QCOMPARE(simulator->program(3), edited);
    QCOMPARE(simulator->program(1), profile().defaultProgram);
    QVERIFY(window->findChild<QAction *>("actionSendToProgram3")->isEnabled());
}

void TestMainWindow::test_answerGoesToTheProgramSelectedAtRequest()
{
    // E2: the user asks, then selects another program before the answer
    // arrives. The answer goes to the program it was asked for.
    const auto window = windowWithPrograms(2);
    QVERIFY(transport->connectTo(simulator->portId()));
    simulator->setProgram(1, withChannel(3));
    const int first = document->store().programs().at(0).id;
    const int second = document->store().programs().at(1).id;
    QListView *view = programsView(*window);
    view->setCurrentIndex(view->model()->index(0, 0));

    window->findChild<QAction *>("actionGetProgram1")->trigger();
    view->setCurrentIndex(view->model()->index(1, 0)); // before the answer
    QVERIFY(QTest::qWaitFor([&] { return document->store().get(first)->program == withChannel(3); },
                            wait_ms));
    QCOMPARE(document->store().get(second)->program, profile().defaultProgram);
}

void TestMainWindow::test_targetDeletedBeforeTheAnswer()
{
    const auto window = windowWithPrograms(2);
    QVERIFY(transport->connectTo(simulator->portId()));
    const int target = document->store().programs().at(1).id;
    window->findChild<QAction *>("actionGetProgram1")->trigger();
    QVERIFY(document->store().remove(target)); // before the answer
    expectWindowShown(); // the message box

    QVERIFY(QTest::qWaitFor([&] { return simulator->pendingAnswers() == 0; }, wait_ms));
    auto *box = window->findChild<QMessageBox *>();
    QVERIFY(box && box->isVisible());
    QVERIFY(box->text().contains(QStringLiteral("deleted")));
    QCOMPARE(document->store().programs().at(0).program, profile().defaultProgram);
}

void TestMainWindow::test_failureShowsOneNonModalMessage()
{
    // A device that never answers: one message, which doesn't block, and the
    // actions are back once the request has failed.
    const auto window = windowWithPrograms(1);
    QVERIFY(transport->connectTo(simulator->portId()));
    simulator->setMode(Lpd8Simulator::Mode::Silent);
    device->setReplyTimeout(std::chrono::milliseconds(50));
    auto *get = window->findChild<QAction *>("actionGetProgram1");

    expectWindowShown(); // the message box, once
    get->trigger();
    QVERIFY(QTest::qWaitFor([&] { return get->isEnabled(); }, wait_ms));
    const QList<QMessageBox *> boxes = window->findChildren<QMessageBox *>();
    QCOMPARE(boxes.size(), 1);
    QVERIFY(boxes.at(0)->isVisible());
    QCOMPARE(boxes.at(0)->windowModality(), Qt::NonModal);
    QVERIFY(boxes.at(0)->text().contains(QStringLiteral("did not answer")));

    // Another failure while it is shown: still one box, one message.
    get->trigger();
    QVERIFY(QTest::qWaitFor([&] { return get->isEnabled(); }, wait_ms));
    QCOMPARE(window->findChildren<QMessageBox *>().size(), 1);
    QCOMPARE(document->store().programs().at(0).program, profile().defaultProgram);
}

void TestMainWindow::test_unsolicitedDumpIsIgnored()
{
    // The security finding: a dump nobody asked for, from the device or
    // from another client, changes nothing.
    const auto window = windowWithPrograms(1);
    QVERIFY(transport->connectTo(simulator->portId()));
    transport->addPort("other:0", "Another client");
    const int steps = document->undoStack().count();
    transport->receive(dumpOf(withChannel(2)), simulator->portId());
    transport->receive(dumpOf(withChannel(2)), "other:0");
    QCOMPARE(document->undoStack().count(), steps);
    QCOMPARE(document->store().programs().at(0).program, profile().defaultProgram);
}

void TestMainWindow::test_midiActionsGoThroughTheTransport()
{
    const auto window = windowWithPrograms(1);
    auto *send = window->findChild<QAction *>("actionSendToProgram2");
    auto *get = window->findChild<QAction *>("actionGetProgram3");
    QVERIFY(send && get);
    // Not connected: the actions are off.
    QVERIFY(!send->isEnabled());
    QVERIFY(!get->isEnabled());

    QVERIFY(transport->connectTo("20:0"));
    QVERIFY(send->isEnabled());
    QVERIFY(get->isEnabled());

    // The codec's bytes, through the device: a send has no answer, so the
    // get goes right after it, and the actions are off until it is answered.
    send->trigger();
    get->trigger();
    const Program &selected = document->store().programs().at(0).program;
    QCOMPARE(transport->sent(),
             (QList<QByteArray>{encodeSendProgram(selected, 2).value(),
                                encodeGetProgram(3).value()}));
    QVERIFY(!send->isEnabled());
    QVERIFY(QTest::qWaitFor([&] { return send->isEnabled(); }, wait_ms));

    // The device goes away: the actions are off again.
    simulator.reset();
    QVERIFY(!send->isEnabled());
}

void TestMainWindow::test_midiOff()
{
    // Without a sequencer, the window still edits programs, and the MIDI
    // actions stay off.
    const MainWindow window(*document, nullptr, nullptr);
    window.findChild<QAction *>("actionNewProgram")->trigger();
    QCOMPARE(document->store().programs().size(), 1);
    programsView(window)->setCurrentIndex(programsView(window)->model()->index(0, 0));
    QVERIFY(!window.findChild<QAction *>("actionSendToProgram1")->isEnabled());
    QVERIFY(!window.findChild<MidiConnectionsModel *>());
}

void TestMainWindow::test_closeAsksTheGuard()
{
    // The window doesn't save on its own (App does, in the guard): with a
    // guard that says no, the close is cancelled and nothing is written.
    const auto window = windowWithPrograms(1);
    show(*window);
    int asked = 0;
    bool allow = false;
    window->setCloseGuard([&asked, &allow] {
        ++asked;
        return allow;
    });
    QVERIFY(!window->close());
    QCOMPARE(asked, 1);
    QVERIFY(window->isVisible());
    QVERIFY(!QFile::exists(path)); // the pending save hasn't run, and close didn't save

    allow = true;
    QVERIFY(window->close());
    QCOMPARE(asked, 2);
    QVERIFY(!window->isVisible());
}

void TestMainWindow::test_problemsShareOneBox()
{
    // A problem while the box is up is added to it, once.
    const auto window = windowWithPrograms(1);
    expectWindowShown(); // the box, once
    window->showProblem(QStringLiteral("First"));
    // A longer text resizes the box, which the plugin reports like a show.
    expectWindowShown();
    window->showProblem(QStringLiteral("Second"));
    window->showProblem(QStringLiteral("Second"));
    // Whole problems are compared: one that is part of another is not a
    // repeat.
    expectWindowShown();
    window->showProblem(QStringLiteral("Sec"));
    const QList<QMessageBox *> boxes = window->findChildren<QMessageBox *>();
    QCOMPARE(boxes.size(), 1);
    QCOMPARE(boxes.at(0)->text(), QStringLiteral("First\n\nSecond\n\nSec"));

    // Once the box is closed, the next problem starts a new list.
    boxes.at(0)->hide();
    expectWindowShown();
    window->showProblem(QStringLiteral("Third"));
    QCOMPARE(boxes.at(0)->text(), QStringLiteral("Third"));
}

void TestMainWindow::test_problemsArePlainText()
{
    // A problem can name a file or a program. A box left to guess would show
    // this as HTML, and load what an <img> tag names (doc/SECURITY.md,
    // finding 6).
    const QString text = QStringLiteral("Cannot write <img src=\"/x\">/programs.json");
    QVERIFY(Qt::mightBeRichText(text));
    const auto window = windowWithPrograms(1);
    expectWindowShown();
    window->showProblem(text);
    const auto *box = window->findChild<QMessageBox *>();
    QVERIFY(box != nullptr);
    QCOMPARE(box->textFormat(), Qt::PlainText);
    QCOMPARE(box->text(), text);
}

void TestMainWindow::test_newProgramUsesTheDefaultSettings()
{
    // Without settings: the factory program, named 'P' and its id.
    {
        const auto window = windowWithPrograms(1);
        const StoredProgram &created = document->store().programs().at(0);
        QCOMPARE(created.name, QStringLiteral("P%1").arg(created.id));
        QCOMPARE(created.program, profile().defaultProgram);
    }

    // With them: the template and the name.
    QSettings().setValue("default/sysex", encodeStoredProgram(withChannel(5)));
    QSettings().setValue("default/name", QStringLiteral("Mine"));
    const auto window = windowWithPrograms(1);
    const StoredProgram &created = document->store().programs().at(1);
    QCOMPARE(created.name, QStringLiteral("Mine"));
    QCOMPARE(created.program, withChannel(5));
}

void TestMainWindow::test_newProgramIgnoresInvalidDefaultSetting()
{
    // A program template in the settings that is not an LPD8 program must
    // not make 'New program' fail silently: the factory program is used.
    QSettings().setValue("default/sysex", QByteArray("not a program"));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Ignoring the setting"));
    const auto window = windowWithPrograms(1);
    QCOMPARE(document->store().programs().size(), 1);
    QCOMPARE(document->store().programs().at(0).program, profile().defaultProgram);
}

QTEST_MAIN(TestMainWindow)

#include "mainwindow_test.moc"
