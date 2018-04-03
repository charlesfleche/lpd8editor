#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "db.h"
#include "commands.h"
#include "iomidi.h"
#include "midiconnectionsmodel.h"
#include "midivaluedelegate.h"
#include "programsmodel.h"
#include "sysexhandler.h"
#include "utils.h"

#include <QApplication>
#include <QComboBox>
#include <QDataWidgetMapper>
#include <QFileDialog>
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QUndoStack>

static const QString SETTINGS_KEY_DEFAULT_NAME = "default/name";
static const QString SETTINGS_KEY_DEFAULT_SYSEX = "default/sysex";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    programsModel(nullptr)
{
    programsModel = new ProgramsModel(this);
    auto io = new IOMidi(this);

    ui->setupUi(this);
    setStatusBar(nullptr);

    auto sysexHandler = new SysexHandler(io);
    auto connectionsModel = new MidiConnectionsModel(io);

    // Undo

    QUndoStack* stack = undoStack();
    Q_CHECK_PTR(stack);

    QAction* undoAction = stack->createUndoAction(this, "&Undo");
    undoAction->setShortcuts(QKeySequence::Undo);

    QAction* redoAction = stack->createRedoAction(this, tr("&Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);

    ui->menuEdit->insertAction(ui->actionNewProgram, redoAction);
    ui->menuEdit->insertAction(redoAction, undoAction);
    ui->menuEdit->insertSeparator(ui->actionNewProgram);

    // Client selection combo box

    QComboBox* clientComboBox = new QComboBox(this);
    clientComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    ui->toolBar->addWidget(clientComboBox);

    ui->newProgramButton->setDefaultAction(ui->actionNewProgram);
    ui->deleteProgramButton->setDefaultAction(ui->actionDeleteProgram);

    ui->programsView->setModel(programsModel);
    ui->programsView->setModelColumn(programModelColumn());

    QDataWidgetMapper *mapper = new QDataWidgetMapper(this);
    mapper->setModel(programsModel);
    mapper->setItemDelegate(new MidiValueDelegate(this));
    mapper->addMapping(ui->channelSpinBox, 2); // XXX

    ui->padsView->setItemDelegate(new MidiValueDelegate(this));
    ui->padsView->setModel(programsModel);
    ui->padsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QHeaderView* headerView = new QHeaderView(Qt::Horizontal, this);
    headerView->setModel(programsModel->padsHeaderModel());
    ui->padsView->setHorizontalHeader(headerView);

    ui->padsView->hideColumn(0);
    ui->padsView->hideColumn(1);

    ui->knobsView->setItemDelegate(new MidiValueDelegate(this));
    ui->knobsView->setModel(programsModel);
    ui->knobsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    headerView = new QHeaderView(Qt::Horizontal, this);
    headerView->setModel(programsModel->knobsHeaderModel());
    ui->knobsView->setHorizontalHeader(headerView);

    ui->knobsView->hideColumn(0);
    ui->knobsView->hideColumn(1);

    // Refresh action delete program

    QItemSelectionModel *sel = ui->programsView->selectionModel();
    Q_CHECK_PTR(sel);
    connect(sel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::refreshUiAccordingToSelection
    );
    connect(sel,
            &QItemSelectionModel::currentRowChanged,
            mapper,
            &QDataWidgetMapper::setCurrentModelIndex);

    connect(sel,
            &QItemSelectionModel::currentRowChanged,
            [=](const QModelIndex &current) {
                Q_CHECK_PTR(programsModel);

                ui->padsView->setRootIndex(programsModel->padsParentIndex(current));
                ui->knobsView->setRootIndex(programsModel->knobsParentIndex(current));
            }
    );

    connect(programsModel,
            &QAbstractItemModel::modelReset,
            this,
            &MainWindow::refreshUiAccordingToSelection
    );

    clientComboBox->setModel(connectionsModel);
    Q_ASSERT(clientComboBox->count() > 0);
    clientComboBox->setEnabled(connectionsModel->connectedPort().isValid());
    connect(connectionsModel,
            &MidiConnectionsModel::connectedPortChanged,
            [=](const QModelIndex &index) {
                clientComboBox->setEnabled(index.isValid());
                if (index.isValid()) {
                    clientComboBox->setCurrentIndex(index.row());
                }
            });

    connect(
        clientComboBox,
        QOverload<int>::of(&QComboBox::activated),
        [=](int row) {
            connectionsModel->connectPort(connectionsModel->index(row));
        }
    );

    connect(sysexHandler,
            &SysexHandler::programReceived,
            [=](const QByteArray& sysex) {
                Q_CHECK_PTR(ui->programsView->selectionModel());
                Q_CHECK_PTR(programsModel);
                Q_CHECK_PTR(undoStack());

                const int programId = currentSelectedProjectId();
                if (programId == -1) {
                    Q_UNIMPLEMENTED();
                } else {
                    UpdateProgramFromSysexCommand* cmd = new UpdateProgramFromSysexCommand(
                        programsModel,
                        programId,
                        sysex
                    );
                    undoStack()->push(cmd);
                }
            });

    ProgramIdSelectionRestorer *restorer = new ProgramIdSelectionRestorer(ui->programsView->selectionModel(), this);
    connect(programsModel,
            &ProgramsModel::modelAboutToBeReset,
            restorer,
            &ProgramIdSelectionRestorer::store);
    connect(programsModel,
            &ProgramsModel::modelReset,
            restorer,
            &ProgramIdSelectionRestorer::restore);

    // Enable MIDI send / receive action when connected

    QList<QAction *> midiSendActions = {
        ui->actionGetProgram1,
        ui->actionGetProgram2,
        ui->actionGetProgram3,
        ui->actionGetProgram4
    };
    for (int i = 0 ; i < midiSendActions.count() ; ++i) {
        connect(
            midiSendActions[i],
            &QAction::triggered,
            [=]() {
                Q_CHECK_PTR(sysexHandler);

                sysexHandler->getProgram(i+1);
            }
        );
    }

    QList<QAction *> midiReceiveActions {
        ui->actionSendToProgram1,
        ui->actionSendToProgram2,
        ui->actionSendToProgram3,
        ui->actionSendToProgram4,
    };
    for (int i = 0 ; i < midiReceiveActions.count() ; ++i) {
        connect(
            midiReceiveActions[i],
            &QAction::triggered,
            [=]() {
                const QByteArray sysex = programsModel->programSysex(currentSelectedProjectId());
                sysexHandler->sendProgram(sysex, i+1);
            }
        );
    }

    QList<QAction *> midiActions = midiSendActions + midiReceiveActions;
    auto midiActionsSetEnabled = [=](){
        for (auto it = midiActions.begin() ; it != midiActions.end() ; ++it) {
            QAction* a = *it;
            Q_CHECK_PTR(a);

            a->setEnabled(connectionsModel->connected() &&
                          ui->programsView->selectionModel()->currentIndex().row() != -1);
        }
    };
    midiActionsSetEnabled();

    connect(
        connectionsModel,
        &MidiConnectionsModel::connectedChanged,
        midiActionsSetEnabled
    );
    connect(
        ui->programsView->selectionModel(),
        &QItemSelectionModel::currentChanged,
        midiActionsSetEnabled
    );
    connect(
        programsModel,
        &ProgramsModel::modelReset,
        midiActionsSetEnabled
    );
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::programModelColumn() const
{
    return 1;
}

QString defaultSysex() {
    return readTextFile(":/default-sysex.sql");
}

void MainWindow::on_actionNewProgram_triggered()
{
    Q_CHECK_PTR(programsModel);
    Q_CHECK_PTR(undoStack());

    QUndoCommand* cmd = new CreateProgramCommand(
        programsModel,
        QSettings().value(SETTINGS_KEY_DEFAULT_NAME).toString(),
        QSettings().value(SETTINGS_KEY_DEFAULT_SYSEX).toByteArray()
    );
    undoStack()->push(cmd);
}

void MainWindow::on_actionDeleteProgram_triggered()
{
    Q_CHECK_PTR(programsModel);
    Q_CHECK_PTR(undoStack());

    QItemSelectionModel* selection_model = ui->programsView->selectionModel();
    Q_CHECK_PTR(selection_model);
    Q_ASSERT(selection_model->currentIndex().row() > -1);

    QUndoCommand *cmd = new DeleteProgramCommand(
        programsModel,
        selectedProgramId(selection_model)
    );
    undoStack()->push(cmd);
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::refreshUiAccordingToSelection()
{
    QItemSelectionModel *sel = ui->programsView->selectionModel();
    Q_CHECK_PTR(sel);

    ui->actionDeleteProgram->setEnabled(sel->hasSelection());

    QWidget* w = sel->currentIndex().row() != -1 ? ui->pageEditor : ui->pageDefault;
    ui->stackedWidget->setCurrentWidget(w);
}

void MainWindow::on_actionImportProgram_triggered()
{
    Q_CHECK_PTR(programsModel);
    Q_CHECK_PTR(undoStack());

    const QString path(
                QFileDialog::getOpenFileName(
                    this,
                    "Import LPD8 program",
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    if (path.isEmpty()) {
        return;
    }

    const QByteArray sysex = fromSysexTextFile(path);
    if (sysex.isNull()) {
        return;
    }

    const QString name = QDir(path).dirName();

    QUndoCommand* cmd = new CreateProgramCommand(
        programsModel,
        name,
        sysex
    );
    undoStack()->push(cmd);
}

void MainWindow::on_actionExportProgram_triggered()
{
    QItemSelectionModel *m = ui->programsView->selectionModel();
    Q_CHECK_PTR(m);

    const int programId = selectedProgramId(m);
    const QString name = programName(programId);
    const QByteArray sysex = programSysex(programId);

    const QDir defaultDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    const QString defaultPath = defaultDir.absoluteFilePath(name);

    const QString path(
                QFileDialog::getSaveFileName(
                    this,
                    "Export LPD8 program",
                    defaultPath));
    if (path.isEmpty()) {
        return;
    }

    writeProgramFile(sysex, path);
}

int MainWindow::currentSelectedProjectId() const {
    Q_CHECK_PTR(ui->programsView->selectionModel());

    return selectedProgramId(ui->programsView->selectionModel());
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(
        this,
        QString("About %1").arg(qApp->applicationName()),
        QString("<h1>%1 v%2</h1><a href=\"%3\">%4</a><p>%5</p><p>%6</p>")
            .arg(qApp->applicationName())
            .arg(qApp->applicationVersion())
            .arg(qApp->property("applicationLink").toString())
            .arg(qApp->property("applicationLink").toString())
            .arg(qApp->property("applicationLicense").toString())
            .arg(qApp->property("applicationCopyright").toString())
    );
}

void MainWindow::on_actionAboutQt_triggered() {
    qApp->aboutQt();
}

