#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "deviceprofile.h"
#include "lpd8codec.h"
#include "lpd8device.h"
#include "midiconnectionsmodel.h"
#include "miditransport.h"
#include "midivaluedelegate.h"
#include "programfile.h"
#include "programmodels.h"
#include "programsdocument.h"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDataWidgetMapper>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QUndoStack>

#include <QtDebug>

using lpd8::ControlsModel;
using lpd8::ProgramListModel;

static const QString SETTINGS_KEY_DEFAULT_NAME = "default/name";
static const QString SETTINGS_KEY_DEFAULT_SYSEX = "default/sysex";

MainWindow::MainWindow(lpd8::ProgramsDocument &document,
                       lpd8::MidiTransport *midi,
                       lpd8::Lpd8Device *device,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_document(document)
    , programsModel(new ProgramListModel(document.store(), document.undoStack(), this))
    , padsModel(new ControlsModel(
              document.store(), document.undoStack(), ControlsModel::Controls::Pads, this))
    , knobsModel(new ControlsModel(
              document.store(), document.undoStack(), ControlsModel::Controls::Knobs, this))
    , m_device(device)
{
    // Both or neither: the device talks through the transport.
    Q_ASSERT((midi == nullptr) == (device == nullptr));
    ui->setupUi(this);
    setStatusBar(nullptr);

    // Undo

    QUndoStack *stack = &m_document.undoStack();

    QAction *undoAction = stack->createUndoAction(this, tr("Undo"));
    undoAction->setShortcuts(QKeySequence::Undo);

    QAction *redoAction = stack->createRedoAction(this, tr("Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);

    ui->menuEdit->insertAction(ui->actionNewProgram, redoAction);
    ui->menuEdit->insertAction(redoAction, undoAction);
    ui->menuEdit->insertSeparator(ui->actionNewProgram);

    // Client selection combo box

    auto *clientComboBox = new QComboBox(this);
    clientComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    ui->toolBar->addWidget(clientComboBox);

    ui->newProgramButton->setDefaultAction(ui->actionNewProgram);
    ui->deleteProgramButton->setDefaultAction(ui->actionDeleteProgram);

    ui->programsView->setModel(programsModel);
    ui->programsView->setModelColumn(ProgramListModel::NameColumn);

    auto *mapper = new QDataWidgetMapper(this);
    mapper->setModel(programsModel);
    mapper->setItemDelegate(new MidiValueDelegate(this));
    mapper->addMapping(ui->channelSpinBox, ProgramListModel::ChannelColumn);

    // Each view has its own flat model, with its own headers: no root index,
    // no hidden columns, and no substitute header views. The columns keep
    // the header's default, interactive mode, as users saw them so far: the
    // old code set 'Stretch' on the headers it then replaced, so it never
    // took effect. Stretched columns would also make the tables ask for less
    // room than their headers need.
    ui->padsView->setItemDelegate(new MidiValueDelegate(this));
    ui->padsView->setModel(padsModel);

    ui->knobsView->setItemDelegate(new MidiValueDelegate(this));
    ui->knobsView->setModel(knobsModel);

    // Selection. The list model is never reset by an edit (only by a load),
    // so the selection model keeps the selected program across renames and
    // across insertions and removals of other rows by itself. When the
    // selected program is removed, the view makes its neighbour current and
    // selected, and the signals below show it (tested in mainwindow_test).

    QItemSelectionModel *sel = ui->programsView->selectionModel();
    connect(sel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::refreshUiAccordingToSelection);
    connect(sel,
            &QItemSelectionModel::currentRowChanged,
            mapper,
            &QDataWidgetMapper::setCurrentModelIndex);
    connect(sel, &QItemSelectionModel::currentRowChanged, this, &MainWindow::showProgram);

    connect(programsModel,
            &QAbstractItemModel::modelReset,
            this,
            &MainWindow::refreshUiAccordingToSelection);

    // Enable MIDI send / receive action when connected

    if (!midi) {
        return; // MIDI is off: the MIDI actions stay disabled
    }

    // MIDI connections

    connectionsModel = new MidiConnectionsModel(*midi, this);

    clientComboBox->setModel(connectionsModel);
    Q_ASSERT(clientComboBox->count() > 0);
    clientComboBox->setEnabled(connectionsModel->connectedPort().isValid());
    connect(connectionsModel,
            &MidiConnectionsModel::connectedPortChanged,
            clientComboBox,
            [clientComboBox](const QModelIndex &index) {
                clientComboBox->setEnabled(index.isValid());
                if (index.isValid()) {
                    clientComboBox->setCurrentIndex(index.row());
                }
            });

    connect(clientComboBox, &QComboBox::activated, connectionsModel, [this](int row) {
        connectionsModel->connectPort(connectionsModel->index(row));
    });

    // MIDI actions

    // 'Get program N' asks the device for its program N, 'Send to program N'
    // writes the selected program into the device's slot N.
    QList<QAction *> getProgramActions = {ui->actionGetProgram1,
                                          ui->actionGetProgram2,
                                          ui->actionGetProgram3,
                                          ui->actionGetProgram4};

    QList<QAction *> sendProgramActions{
            ui->actionSendToProgram1,
            ui->actionSendToProgram2,
            ui->actionSendToProgram3,
            ui->actionSendToProgram4,
    };

    const QList<QAction *> midiActions = getProgramActions + sendProgramActions;

    // Connected below with connectionsModel as the context: ~MainWindow
    // deletes it first, while 'ui' still exists, so the connections end
    // before anything the lambda uses is gone. Off while a request is in
    // flight: one at a time.
    auto midiActionsSetEnabled = [this, midiActions]() {
        for (QAction *a : midiActions) {
            a->setEnabled(connectionsModel->connected()
                          && ui->programsView->selectionModel()->currentIndex().row() != -1
                          && !m_device->isBusy());
        }
    };

    midiActionsSetEnabled();

    connect(connectionsModel,
            &MidiConnectionsModel::connectedChanged,
            connectionsModel,
            midiActionsSetEnabled);

    connect(ui->programsView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            connectionsModel,
            midiActionsSetEnabled);

    connect(programsModel,
            &QAbstractItemModel::modelReset,
            connectionsModel,
            midiActionsSetEnabled);

    connect(m_device, &lpd8::Lpd8Device::busyChanged, connectionsModel, midiActionsSetEnabled);

    // The device: its signals may come from inside requestProgram(), so they
    // are connected before any request (lpd8device.h).

    connect(m_device, &lpd8::Lpd8Device::programReceived, this, &MainWindow::programReceived);
    connect(m_device, &lpd8::Lpd8Device::sendDone, this, &MainWindow::requestEnded);
    connect(m_device, &lpd8::Lpd8Device::requestFailed, this, [this](int, const QString &reason) {
        requestFailed(reason);
    });

    for (int i = 0; i < getProgramActions.count(); ++i) {
        connect(getProgramActions[i], &QAction::triggered, this, [this, i]() {
            getProgram(i + 1);
        });
    }

    for (int i = 0; i < sendProgramActions.count(); ++i) {
        connect(sendProgramActions[i], &QAction::triggered, this, [this, i]() {
            sendProgram(i + 1);
        });
    }
}

void MainWindow::getProgram(int slot)
{
    const int target = currentProgramId();
    if (target == -1 || pending) {
        return; // the actions are disabled then
    }
    // Set before the request: its answer, or its failure, may come before
    // requestProgram() returns.
    pending = PendingRequest{.slot = slot, .targetProgramId = target};
    m_device->requestProgram(slot);
}

void MainWindow::sendProgram(int slot)
{
    const lpd8::StoredProgram *stored = m_document.store().get(currentProgramId());
    if (!stored || pending) {
        return; // the actions are disabled then
    }
    pending = PendingRequest{.slot = slot, .targetProgramId = -1};
    m_device->sendProgram(slot, stored->program);
}

void MainWindow::programReceived(int slot, const lpd8::Program &program)
{
    Q_ASSERT(pending && pending->slot == slot);
    const int target = pending ? pending->targetProgramId : -1;
    requestEnded();
    // The program selected when the user asked, not the one selected now.
    // It may have been deleted while the device answered.
    if (!m_document.store().get(target)) {
        showProblem(tr("The program received from the LPD8 was not applied: the program it "
                       "was for has been deleted."));
        return;
    }
    push(lpd8::makeSetProgramCommand(m_document.store(), target, program),
         "apply a received program");
}

void MainWindow::requestEnded()
{
    pending.reset();
}

void MainWindow::requestFailed(const QString &reason)
{
    requestEnded();
    showProblem(reason);
}

void MainWindow::showProblem(const QString &text)
{
    if (!problemBox) {
        problemBox = new QMessageBox(QMessageBox::Warning,
                                     QApplication::applicationName(),
                                     QString(),
                                     QMessageBox::Ok,
                                     this);
        // Never modal, never exec(): this is called from the device's
        // signals, which may come from inside its request loop, and from
        // the document's save timer.
        problemBox->setWindowModality(Qt::NonModal);
    }
    if (!problemBox->isVisible()) {
        m_problems.clear(); // the user has read the previous ones
    }
    // One box for problems that come together. Each new one is added, so
    // none is lost; one already shown isn't repeated (an unplug fails every
    // queued request with the same reason). Whole problems are compared, not
    // substrings: a short problem that happens to be part of a longer one
    // is still shown.
    if (!m_problems.contains(text)) {
        m_problems.append(text);
        problemBox->setText(m_problems.join(QStringLiteral("\n\n")));
    }
    problemBox->show();
}

void MainWindow::setCloseGuard(CloseGuard guard)
{
    m_closeGuard = std::move(guard);
}

MainWindow::~MainWindow()
{
    // Teardown order. After this body, 'ui' is destroyed, and only then do
    // the base class destructors delete the children. Signals that children
    // emit while being deleted would still reach this window's slots and
    // lambdas, which use 'ui'. So, while 'ui' still exists, remove every
    // object outside the window's widgets that connects into them:
    //
    // - the MIDI objects (lambdas using 'ui' and the store). The transport
    //   and the device outlive the window (App owns them), so their
    //   connections into the window end only when they are broken, here:
    //   the connections model is deleted, and the device disconnected.
    //   (App clears the document's save-failure handler, which shows
    //   problems in this window, before destroying it.)
    // - the selection model's connections to this window (showProgram,
    //   refreshUiAccordingToSelection)
    // - the models (modelReset -> refreshUiAccordingToSelection, which uses
    //   'ui'). The views and the mapper let go of a model when it is
    //   destroyed.
    //
    // tests/mainwindow_test.cpp makes signals fire during the teardown to
    // check this.
    if (m_device) {
        m_device->disconnect(this);
        m_device = nullptr;
    }
    delete connectionsModel;
    connectionsModel = nullptr;
    ui->programsView->selectionModel()->disconnect(this);
    delete programsModel;
    programsModel = nullptr;
    delete padsModel;
    padsModel = nullptr;
    delete knobsModel;
    knobsModel = nullptr;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // The window doesn't save itself: App does the final save, and may let
    // the user cancel the close if it fails.
    if (m_closeGuard && !m_closeGuard()) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::on_actionNewProgram_triggered()
{
    const QSettings settings;
    // An optional template for new programs. If it is not an LPD8 program,
    // it is ignored and the factory program is used.
    lpd8::Program program = lpd8::profile().defaultProgram;
    const QByteArray sysex = settings.value(SETTINGS_KEY_DEFAULT_SYSEX).toByteArray();
    if (!sysex.isNull()) {
        if (const auto decoded = lpd8::decodeProgram(sysex)) {
            program = decoded->program;
        } else {
            qWarning() << "Ignoring the setting" << SETTINGS_KEY_DEFAULT_SYSEX
                       << ": it is not an LPD8 program";
        }
    }
    // Without the setting, 'P' and the program's id, as before. A setting
    // that is set, even empty, is used as it is.
    QString name = settings.value(SETTINGS_KEY_DEFAULT_NAME).toString();
    if (name.isNull()) {
        name = QStringLiteral("P%1").arg(m_document.store().nextId());
    }
    push(lpd8::makeAddCommand(m_document.store(), name, program), "create a program");
}

void MainWindow::on_actionDeleteProgram_triggered()
{
    const int programId = currentProgramId();
    if (programId == -1) {
        return; // nothing selected (the action is disabled then)
    }
    push(lpd8::makeRemoveCommand(m_document.store(), programId), "delete a program");
}

void MainWindow::on_actionQuit_triggered()
{
    // quit() first asks every window to close (Qt 6), so closeEvent() and
    // its guard run; then App saves at aboutToQuit.
    qApp->quit();
}

void MainWindow::refreshUiAccordingToSelection()
{
    QItemSelectionModel *sel = ui->programsView->selectionModel();

    ui->actionDeleteProgram->setEnabled(sel->hasSelection());
    ui->actionExportProgram->setEnabled(sel->hasSelection());

    QWidget *w = sel->currentIndex().row() != -1 ? ui->pageEditor : ui->pageDefault;
    ui->stackedWidget->setCurrentWidget(w);
}

void MainWindow::showProgram(const QModelIndex &current)
{
    const int id = programsModel->programId(current.row());
    padsModel->setProgramId(id);
    knobsModel->setProgramId(id);
}

void MainWindow::push(lpd8::CommandOrRefusal command, const char *what)
{
    if (!command) {
        qWarning() << "Cannot" << what << ": the change was refused";
        return;
    }
    if (*command) { // nullptr: the change changes nothing
        m_document.undoStack().push(command->release());
    }
}

QString MainWindow::programFileFilter() const
{
    // Files exported by older versions have no extension, hence 'All files'.
    return tr("LPD8 programs (*.lpd8);;All files (*)");
}

void MainWindow::on_actionImportProgram_triggered()
{
    const QString title = tr("Import LPD8 program");
    const QString path = QFileDialog::getOpenFileName(
            this,
            title,
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            programFileFilter());
    if (path.isEmpty()) {
        return;
    }

    const auto imported = lpd8::importProgramFile(path);
    if (!imported) {
        QMessageBox::warning(this, title, imported.error());
        return;
    }
    push(lpd8::makeAddCommand(m_document.store(), imported->name, imported->program),
         "import a program");
}

void MainWindow::on_actionExportProgram_triggered()
{
    const QString title = tr("Export LPD8 program");

    const lpd8::StoredProgram *stored = m_document.store().get(currentProgramId());
    if (!stored) {
        return; // nothing selected (the action is disabled then)
    }
    // Copied: the dialog runs an event loop, in which the store may change.
    const lpd8::StoredProgram program = *stored;

    const QDir defaultDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    const QString defaultPath = defaultDir.absoluteFilePath(program.name + QStringLiteral(".lpd8"));

    const QString path =
            QFileDialog::getSaveFileName(this, title, defaultPath, programFileFilter());
    if (path.isEmpty()) {
        return;
    }

    if (const auto written = lpd8::exportProgramFile(program.program, path); !written) {
        QMessageBox::warning(this, title, written.error());
    }
}

int MainWindow::currentProgramId() const
{
    return programsModel->programId(ui->programsView->selectionModel()->currentIndex().row());
}

void MainWindow::on_actionAbout_triggered()
{
    // One multi-argument arg() call: chained .arg().arg() calls each scan the
    // whole string again, so a '%1' inside a substituted value would be
    // replaced by the next call. The values go into rich text, so they are
    // HTML-escaped.
    const QString name = QApplication::applicationName().toHtmlEscaped();
    const QString link = qApp->property("applicationLink").toString().toHtmlEscaped();
    QMessageBox::about(
            this,
            tr("About %1").arg(QApplication::applicationName()),
            QStringLiteral("<h1>%1</h1><h2>%2</h2><a href=\"%3\">%3</a><p>%4</p><p>%5</p>")
                    .arg(name,
                         QApplication::applicationVersion().toHtmlEscaped(),
                         link,
                         qApp->property("applicationLicense").toString().toHtmlEscaped(),
                         qApp->property("applicationCopyright").toString().toHtmlEscaped()));
}
