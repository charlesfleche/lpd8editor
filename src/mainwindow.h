#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ProgramsModel;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionNewProgram_triggered();
    void on_actionDeleteProgram_triggered();
    void on_actionQuit_triggered();

    void on_actionImportProgram_triggered();
    void on_actionExportProgram_triggered();

    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();

    void refreshUiAccordingToSelection();

private:
    int currentSelectedProjectId() const;
    int programModelColumn() const;

    Ui::MainWindow *ui;
    ProgramsModel* programsModel;
};

#endif // MAINWINDOW_H
