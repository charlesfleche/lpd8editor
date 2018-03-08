#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "application.h"
#include "midiio.h"
#include "midiportsmodel.h"
#include "programproxymodel.h"
#include "utils.h"

#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QStandardItemModel>

#include <QtDebug>

MainWindow::MainWindow(Application* app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    app(app)
{
    Q_CHECK_PTR(app);
    Q_CHECK_PTR(app->midiIO());

    ui->setupUi(this);
    setStatusBar(Q_NULLPTR);

    QComboBox* clientComboBox = new QComboBox(this);
    ui->toolBar->addWidget(clientComboBox);
    ui->toolBar->addAction(ui->actionRescan);

    ui->newProgramButton->setDefaultAction(ui->actionNewProgram);
    ui->deleteProgramButton->setDefaultAction(ui->actionDeleteProgram);

    ProgramProxyModel* programsProxyModel = new ProgramProxyModel(this);
    programsProxyModel->setSourceModel(app->programs());
    programsProxyModel->setActiveProgramId(app->activeProgramId());
    ui->programsView->setModel(programsProxyModel);

    connect(app,
            &Application::activeProgramIdChanged,
            [=](int programId) {
                programsProxyModel->setActiveProgramId(programId);
                refreshWidgetStack();
            });

    ui->programsView->setModelColumn(programModelColumn());

    ui->padsView->setModel(app->pads());
    ui->padsView->hideColumn(0);
    ui->padsView->hideColumn(1);
    ui->padsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->knobsView->setModel(app->knobs());
    ui->knobsView->hideColumn(0);
    ui->knobsView->hideColumn(1);
    ui->knobsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(app->programs(),
            &QAbstractItemModel::modelReset,
            this,
            &MainWindow::refreshActionDeleteProgram
    );

    refreshActionDeleteProgram();
    refreshWidgetStack();

    QGridLayout* l = new QGridLayout;
    l->setSizeConstraint(QLayout::SetMinimumSize);
    for (int i=1 ; i <= 16 ; ++i) {
        QPushButton* b = new QPushButton(this);
        b->setMaximumWidth(b->height()*3);
        b->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        b->setText(QString::number(i));
        b->setCheckable(true);
        b->setAutoExclusive(true);
        connect(b,
                &QPushButton::clicked,
                [=](){
            Q_CHECK_PTR(app);
            app->setActiveProgramChannel(i);
        });
        const int row = (i - 1) < 8 ? 0 : 1;
        const int col = (i - 1) % 8;
        l->addWidget(b, row, col, 1, 1);
        midiChannelButtons.append(b);
    }

    connect(app,
            &Application::activeProgramChannelChanged,
            this,
            &MainWindow::setMidiChannel);

    ui->groupBoxMidiChannel->setLayout(l);

    connect(app->midiIO(),
            &MidiIO::thirdPartyModifiedConnections,
            [=]() {
                clientComboBox->disconnect();
                clientComboBox->setModel(new QStandardItemModel(this));
                clientComboBox->addItem("Managed by third party");
                clientComboBox->setEnabled(false);
                ui->actionRescan->setEnabled(false);
            }
    );

    connect(clientComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int row) {
                Q_CHECK_PTR(app->midiIO());

                QAbstractItemModel* midiPortsModel = app->midiIO()->midiPortsModel();
                const QModelIndex index(midiPortsModel->index(row, 0));
                app->midiIO()->connectPort(index);
            });

    clientComboBox->setModel(app->midiIO()->midiPortsModel());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setMidiChannel(int channel)
{
    Q_ASSERT(channel == -1 || (channel >= 1 && channel <= 16));

    QPushButton* b;
    if (channel == -1) {
        foreach (b, midiChannelButtons) {
            b->setChecked(false);
        }
    } else {
        midiChannelButtons[--channel]->setChecked(true);
    }
}

int MainWindow::programModelColumn() const
{
    return 1;
}

void MainWindow::refreshWidgetStack() {
    Q_CHECK_PTR(app);

    QWidget* w = app->activeProgramId() > 0 ? ui->pageEditor : ui->pageDefault;
    ui->stackedWidget->setCurrentWidget(w);
}

void MainWindow::on_actionNewProgram_triggered()
{
    Q_CHECK_PTR(app);

    app->newProgram("New program");
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_programsView_activated(const QModelIndex& idx)
{
    Q_ASSERT(idx.isValid());
    Q_CHECK_PTR(idx.model());
    Q_CHECK_PTR(app);

    app->setActiveProgramId(getProgramId(idx.model(), idx.row()));
}

void MainWindow::on_actionDeleteProgram_triggered()
{
    Q_CHECK_PTR(app);

    app->deleteProgram(app->activeProgramId());
}

void MainWindow::refreshActionDeleteProgram()
{
    Q_CHECK_PTR(app);

    ui->actionDeleteProgram->setEnabled(app->programs()->rowCount() > 0);
}

void MainWindow::on_actionImportProgram_triggered()
{
    Q_CHECK_PTR(app);

    const QString path(
                QFileDialog::getOpenFileName(
                    this,
                    "Import LPD8 program",
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    if (path.isEmpty()) {
        return;
    }
    app->importProgram(path);
}

void MainWindow::on_actionExportProgram_triggered()
{
    Q_CHECK_PTR(app);

    const QString path(
                QFileDialog::getSaveFileName(
                    this,
                    "Export LPD8 program",
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    if (path.isEmpty()) {
        return;
    }
    app->exportActiveProgram(path);
}

void MainWindow::on_actionGetProgram1_triggered()
{
    Q_CHECK_PTR(app);

    app->fetchProgram(1);
}

void MainWindow::on_actionGetProgram2_triggered()
{
    Q_CHECK_PTR(app);

    app->fetchProgram(2);
}

void MainWindow::on_actionGetProgram3_triggered()
{
    Q_CHECK_PTR(app);

    app->fetchProgram(3);
}

void MainWindow::on_actionGetProgram4_triggered()
{
    Q_CHECK_PTR(app);

    app->fetchProgram(4);
}

void MainWindow::on_actionSendToProgram1_triggered()
{
    Q_CHECK_PTR(app);

    app->sendProgram(1);
}

void MainWindow::on_actionSendToProgram2_triggered()
{
    Q_CHECK_PTR(app);

    app->sendProgram(2);
}

void MainWindow::on_actionSendToProgram3_triggered()
{
    Q_CHECK_PTR(app);

    app->sendProgram(3);
}

void MainWindow::on_actionSendToProgram4_triggered()
{
    Q_CHECK_PTR(app);

    app->sendProgram(4);
}

void MainWindow::on_actionRescan_triggered() {
    Q_CHECK_PTR(app->midiIO());

    app->midiIO()->rescanPorts();
}

//void MainWindow::on_listView_activated(const QModelIndex& index) {
//    Q_CHECK_PTR(app);

//    MidiPortsModel* portsModel = qobject_cast<MidiPortsModel*>(app->midiIO()->midiPortsModel());
//    Q_CHECK_PTR(portsModel);

//    portsModel->connect(index);
//}
