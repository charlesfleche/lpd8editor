#include "db.h"
#include "mainwindow.h"
#include "midiconnectionsmodel.h"
#include "programsmodel.h"

#include <QAction>
#include <QItemSelectionModel>
#include <QListView>
#include <QPointer>
#include <QSettings>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QtTest>

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_destroyWithSelectedProgram();
    void test_newProgramIgnoresInvalidDefaultSetting();

private:
    QTemporaryDir configDir;
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
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    QVERIFY(db.open());
    QVERIFY(!initDb(db).isValid());
}

void TestMainWindow::cleanup()
{
    QSqlDatabase::database().close();
}

void TestMainWindow::test_destroyWithSelectedProgram()
{
    // Destroying the window runs ~MainWindow, which destroys 'ui', and then
    // the base class destructors, which delete the children (models, views,
    // MIDI objects). Signals emitted during that teardown must not reach
    // slots or lambdas that use 'ui'. With an ALSA sequencer, the MIDI
    // connections are made and exercised too. Run under ASan to check.
    auto *window = new MainWindow(QSqlDatabase::database());
    window->show();

    auto *newProgram = window->findChild<QAction *>("actionNewProgram");
    QVERIFY(newProgram);
    newProgram->trigger();
    newProgram->trigger();

    auto *view = window->findChild<QListView *>("programsView");
    QVERIFY(view);
    QCOMPARE(view->model()->rowCount(), 2);
    view->setCurrentIndex(view->model()->index(1, view->modelColumn()));
    QVERIFY(view->currentIndex().isValid());

    // Make signals fire during the teardown, through the window's own
    // connections. The programs model is the window's first child: unless
    // ~MainWindow deletes it itself, it is deleted first in the children
    // teardown, after 'ui' is gone, while the selection model and the MIDI
    // model still exist. At that moment, emit what those objects can emit
    // (Qt signals are public functions). Every slot or lambda of the window
    // they reach must already be disconnected, or ASan reports the freed
    // 'ui'. QPointer: each emitter may already be gone, which is fine.
    auto *programs = window->findChild<ProgramsModel *>();
    QVERIFY(programs);
    const QPointer<QItemSelectionModel> selection = view->selectionModel();
    const QPointer<MidiConnectionsModel> connections = window->findChild<MidiConnectionsModel *>();
    if (!connections) {
        qInfo("No ALSA sequencer: the MIDI connections are not exercised");
    }
    connect(programs, &QObject::destroyed, this, [selection, connections] {
        if (selection) {
            emit selection->currentRowChanged({}, {});
            emit selection->selectionChanged({}, {});
        }
        if (connections) {
            emit connections->connectedChanged(true);
            emit connections->connectedPortChanged({});
        }
    });

    delete window;
}

void TestMainWindow::test_newProgramIgnoresInvalidDefaultSetting()
{
    // A program template in the settings that is not an LPD8 program must
    // not make 'New program' fail silently: the factory program is used.
    QSettings().setValue("default/sysex", QByteArray("not a program"));

    const MainWindow window(QSqlDatabase::database());
    auto *newProgram = window.findChild<QAction *>("actionNewProgram");
    auto *view = window.findChild<QListView *>("programsView");
    QVERIFY(newProgram && view);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Ignoring the setting"));
    newProgram->trigger();
    QCOMPARE(view->model()->rowCount(), 1);

    QSettings().remove("default/sysex");
}

QTEST_MAIN(TestMainWindow)

#include "mainwindow_test.moc"
