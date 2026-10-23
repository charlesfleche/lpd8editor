#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "deviceprofile.h"
#include "lpd8codec.h"
#include "midiconnectionsmodel.h"
#include "miditransport.h"
#include "midivaluedelegate.h"
#include "programfile.h"
#include "programmodels.h"
#include "programsdocument.h"
#include "sysexhandler.h"

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

MainWindow::MainWindow(lpd8::ProgramsDocument &document, lpd8::MidiTransport *midi, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_document(document)
    , programsModel(new ProgramListModel(document.store(), document.undoStack(), this))
    , padsModel(new ControlsModel(
              document.store(), document.undoStack(), ControlsModel::Controls::Pads, this))
    , knobsModel(new ControlsModel(
              document.store(), document.undoStack(), ControlsModel::Controls::Knobs, this))
{
    ui->setupUi(this);
    setStatusBar(nullptr);

    // A failed save is shown once (ProgramsDocument), with what it means.
    m_document.setSaveFailureHandler([this](const QString &error) {
        QMessageBox::warning(this,
                             tr("Cannot save the programs"),
                             tr("%1\n\nYour changes are not saved yet. lpd8editor will "
                                "try again at the next change, and when it quits.")
                                     .arg(error));
    });

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
    // before anything the lambda uses is gone.
    auto midiActionsSetEnabled = [this, midiActions]() {
        for (QAction *a : midiActions) {
            a->setEnabled(connectionsModel->connected()
                          && ui->programsView->selectionModel()->currentIndex().row() != -1);
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

    // Sysex

    sysexHandler = new SysexHandler(*midi, this);

    connect(sysexHandler, &SysexHandler::programReceived, this, &MainWindow::applyProgramDump);

    for (int i = 0; i < getProgramActions.count(); ++i) {
        connect(getProgramActions[i], &QAction::triggered, sysexHandler, [this, i]() {
            sysexHandler->getProgram(i + 1);
        });
    }

    for (int i = 0; i < sendProgramActions.count(); ++i) {
        connect(sendProgramActions[i], &QAction::triggered, sysexHandler, [this, i]() {
            const lpd8::StoredProgram *stored = m_document.store().get(currentProgramId());
            if (!stored) {
                return; // nothing selected (the actions are disabled then)
            }
            // The same bytes as before the switch: the storage
            // format, which sendProgram() turns into a request.
            sysexHandler->sendProgram(lpd8::encodeStoredProgram(stored->program), i + 1);
        });
    }
}

MainWindow::~MainWindow()
{
    // Teardown order. After this body, 'ui' is destroyed, and only then do
    // the base class destructors delete the children. Signals that children
    // emit while being deleted would still reach this window's slots and
    // lambdas, which use 'ui'. So, while 'ui' still exists, remove every
    // object outside the window's widgets that connects into them:
    //
    // - the save failure handler, which shows a dialog over this window
    // - the MIDI objects (lambdas using 'ui' and the store). The transport
    //   outlives the window, so its connections into them end only when
    //   they are deleted, here.
    // - the selection model's connections to this window (showProgram,
    //   refreshUiAccordingToSelection)
    // - the models (modelReset -> refreshUiAccordingToSelection, which uses
    //   'ui'). The views and the mapper let go of a model when it is
    //   destroyed.
    //
    // tests/mainwindow_test.cpp makes signals fire during the teardown to
    // check this.
    m_document.setSaveFailureHandler({});
    delete sysexHandler;
    sysexHandler = nullptr;
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

void MainWindow::applyProgramDump(const QByteArray &sysex)
{
    const int programId = currentProgramId();
    if (programId == -1) {
        Q_UNIMPLEMENTED();
        return;
    }
    const auto decoded = lpd8::decodeProgram(sysex);
    if (!decoded) {
        qWarning() << "Ignoring a program dump:" << lpd8::describe(decoded.error());
        return;
    }
    push(lpd8::makeSetProgramCommand(m_document.store(), programId, decoded->program),
         "apply a program dump");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_document.saveNow();
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
    // QCoreApplication::aboutToQuit saves (main.cpp).
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
