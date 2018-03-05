#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class Application;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Application*, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionNewProgram_triggered();
    void on_actionDeleteProgram_triggered();
    void on_actionQuit_triggered();

    void on_programsView_activated(const QModelIndex&);

    void on_actionImportProgram_triggered();
    void on_actionExportProgram_triggered();

    void on_actionGetProgram1_triggered();
    void on_actionGetProgram2_triggered();
    void on_actionGetProgram3_triggered();
    void on_actionGetProgram4_triggered();

    void on_actionSendToProgram1_triggered();
    void on_actionSendToProgram2_triggered();
    void on_actionSendToProgram3_triggered();
    void on_actionSendToProgram4_triggered();

    void on_listView_activated(const QModelIndex&);
    void on_rescanPortsButton_clicked();

    void setMidiChannel(int);

    void refreshActionDeleteProgram();
    void refreshWidgetStack();

private:
    int programModelColumn() const;

    Ui::MainWindow *ui;
    Application* app;
    QList<QPushButton*> midiChannelButtons;
};

#endif // MAINWINDOW_H
