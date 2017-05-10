#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "application.h"

MainWindow::MainWindow(Application* app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    app(app)
{
    Q_CHECK_PTR(app);

    ui->setupUi(this);

    ui->programsView->setModel(app->programs());
    ui->programsView->setModelColumn(programModelColumn());
    ui->padsView->setModel(app->pads());
    ui->knobsView->setModel(app->knobs());
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::programModelColumn() const
{
    return 1;
}
