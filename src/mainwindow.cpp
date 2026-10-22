#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "commands.h"
#include "db.h"
#include "iomidi.h"
#include "lpd8_sysex.h"
#include "midiconnectionsmodel.h"
#include "midivaluedelegate.h"
#include "programsmodel.h"
#include "sysexhandler.h"
#include "utils.h"

#include <QApplication>
#include <QComboBox>
#include <QDataWidgetMapper>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QUndoStack>

#include <QtDebug>

static const QString SETTINGS_KEY_DEFAULT_NAME = "default/name";
static const QString SETTINGS_KEY_DEFAULT_SYSEX = "default/sysex";

MainWindow::MainWindow(const QSqlDatabase &db, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , programsModel(new ProgramsModel(this, db))
{
    ui->setupUi(this);
    setStatusBar(nullptr);

    // Undo

    QUndoStack *stack = undoStack();

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
    ui->programsView->setModelColumn(ProgramColumn::Name);

    auto *mapper = new QDataWidgetMapper(this);
    mapper->setModel(programsModel);
    mapper->setItemDelegate(new MidiValueDelegate(this));
    mapper->addMapping(ui->channelSpinBox, ProgramColumn::Channel);

    ui->padsView->setItemDelegate(new MidiValueDelegate(this));
    ui->padsView->setModel(programsModel);
    ui->padsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto *headerView = new QHeaderView(Qt::Horizontal, this);
    headerView->setModel(programsModel->padsHeaderModel());
    ui->padsView->setHorizontalHeader(headerView);

    ui->padsView->hideColumn(PadColumn::ProgramId);
    ui->padsView->hideColumn(PadColumn::ControlId);

    ui->knobsView->setItemDelegate(new MidiValueDelegate(this));
    ui->knobsView->setModel(programsModel);
    ui->knobsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    headerView = new QHeaderView(Qt::Horizontal, this);
    headerView->setModel(programsModel->knobsHeaderModel());
    ui->knobsView->setHorizontalHeader(headerView);

    ui->knobsView->hideColumn(KnobColumn::ProgramId);
    ui->knobsView->hideColumn(KnobColumn::ControlId);

    // Refresh action delete program

    QItemSelectionModel *sel = ui->programsView->selectionModel();
    connect(sel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::refreshUiAccordingToSelection);
    connect(sel,
            &QItemSelectionModel::currentRowChanged,
            mapper,
            &QDataWidgetMapper::setCurrentModelIndex);

    connect(sel, &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex &current) {
        ui->padsView->setRootIndex(programsModel->padsParentIndex(current));
        ui->knobsView->setRootIndex(programsModel->knobsParentIndex(current));
    });

    connect(programsModel,
            &QAbstractItemModel::modelReset,
            this,
            &MainWindow::refreshUiAccordingToSelection);

    auto *restorer = new ProgramIdSelectionRestorer(ui->programsView->selectionModel(), this);
    connect(programsModel,
            &ProgramsModel::modelAboutToBeReset,
            restorer,
            &ProgramIdSelectionRestorer::store);
    connect(programsModel,
            &ProgramsModel::modelReset,
            restorer,
            &ProgramIdSelectionRestorer::restore);

    // Enable MIDI send / receive action when connected

    IOMidi *io = nullptr;
    try {
        io = new IOMidi(this);
        midi = io;
    } catch (const std::runtime_error &e) {
        qWarning() << "Disabling MIDI: " << e.what();
        return;
    }

    // MIDI connections

    auto *connectionsModel = new MidiConnectionsModel(io);

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

    connect(clientComboBox, &QComboBox::activated, connectionsModel, [connectionsModel](int row) {
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

    // Connected below with connectionsModel as the context: it is the
    // shortest-lived object the lambda uses (a child of 'io'), so the
    // connections end when it is destroyed.
    auto midiActionsSetEnabled = [this, midiActions, connectionsModel]() {
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

    connect(programsModel, &ProgramsModel::modelReset, connectionsModel, midiActionsSetEnabled);

    // Sysex

    auto *sysexHandler = new SysexHandler(io);

    connect(sysexHandler, &SysexHandler::programReceived, this, [this](const QByteArray &sysex) {
        const int programId = currentSelectedProjectId();
        if (programId == -1) {
            Q_UNIMPLEMENTED();
        } else {
            auto *cmd = new UpdateProgramFromSysexCommand(programsModel, programId, sysex);
            undoStack()->push(cmd);
        }
    });

    for (int i = 0; i < getProgramActions.count(); ++i) {
        connect(getProgramActions[i], &QAction::triggered, sysexHandler, [sysexHandler, i]() {
            sysexHandler->getProgram(i + 1);
        });
    }

    for (int i = 0; i < sendProgramActions.count(); ++i) {
        connect(sendProgramActions[i],
                &QAction::triggered,
                sysexHandler,
                [this, sysexHandler, i]() {
                    const QByteArray sysex =
                            programsModel->programSysex(currentSelectedProjectId());
                    sysexHandler->sendProgram(sysex, i + 1);
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
    // - the MIDI objects (lambdas using 'ui' and programsModel)
    // - the selection model's connections to this window (the
    //   currentRowChanged lambda, refreshUiAccordingToSelection)
    // - the programs model (modelReset -> refreshUiAccordingToSelection,
    //   and the lambdas above)
    //
    // tests/mainwindow_test.cpp makes signals fire during the teardown to
    // check this.
    delete midi;
    midi = nullptr;
    ui->programsView->selectionModel()->disconnect(this);
    delete programsModel;
    programsModel = nullptr;
}

void MainWindow::on_actionNewProgram_triggered()
{
    const QSettings settings;
    // An optional template for new programs. If it is not an LPD8 program,
    // creating the program would fail without a word to the user, so it is
    // ignored and the factory program is used.
    QByteArray sysex = settings.value(SETTINGS_KEY_DEFAULT_SYSEX).toByteArray();
    if (!sysex.isNull() && !sysex::isProgram(sysex)) {
        qWarning() << "Ignoring the setting" << SETTINGS_KEY_DEFAULT_SYSEX
                   << ": it is not an LPD8 program";
        sysex = QByteArray();
    }
    undoStack()->push(new CreateProgramCommand(
            programsModel, settings.value(SETTINGS_KEY_DEFAULT_NAME).toString(), sysex));
}

void MainWindow::on_actionDeleteProgram_triggered()
{
    QItemSelectionModel *selection_model = ui->programsView->selectionModel();
    Q_ASSERT(selection_model->currentIndex().row() > -1);

    QUndoCommand *cmd = new DeleteProgramCommand(programsModel, selectedProgramId(selection_model));
    undoStack()->push(cmd);
}

void MainWindow::on_actionQuit_triggered()
{
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

    const std::expected<QByteArray, QString> sysex = fromSysexTextFile(path);
    if (!sysex) {
        QMessageBox::warning(this, title, sysex.error());
        return;
    }
    if (!sysex::isProgram(*sysex)) {
        QMessageBox::warning(
                this, title, tr("%1 is not an LPD8 program.").arg(QDir::toNativeSeparators(path)));
        return;
    }

    // 'Drums.lpd8' becomes the program 'Drums'.
    const QString name = QFileInfo(path).completeBaseName();
    undoStack()->push(new CreateProgramCommand(programsModel, name, *sysex));
}

void MainWindow::on_actionExportProgram_triggered()
{
    const QString title = tr("Export LPD8 program");

    const int programId = currentSelectedProjectId();
    if (!programsModel->programIndex(programId).isValid()) {
        return; // nothing selected (the action is disabled then)
    }
    const QString name = programsModel->programName(programId);
    const QByteArray sysex = programsModel->programSysex(programId);
    if (sysex.isEmpty()) {
        QMessageBox::warning(this, title, tr("Cannot read the program %1.").arg(name));
        return;
    }

    const QDir defaultDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    const QString defaultPath = defaultDir.absoluteFilePath(name + QStringLiteral(".lpd8"));

    const QString path =
            QFileDialog::getSaveFileName(this, title, defaultPath, programFileFilter());
    if (path.isEmpty()) {
        return;
    }

    if (const auto written = writeProgramFile(sysex, path); !written) {
        QMessageBox::warning(this, title, written.error());
    }
}

int MainWindow::currentSelectedProjectId() const
{
    return selectedProgramId(ui->programsView->selectionModel());
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
