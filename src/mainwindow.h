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
    Q_DISABLE_COPY_MOVE(MainWindow)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_actionNewProgram_triggered();
    void on_actionDeleteProgram_triggered();
    void on_actionQuit_triggered();

    void on_actionImportProgram_triggered();
    void on_actionExportProgram_triggered();

    void on_actionAbout_triggered();

    void refreshUiAccordingToSelection();

private:
    int currentSelectedProjectId() const;
    int programModelColumn() const;

    Ui::MainWindow *ui;
    ProgramsModel *programsModel{nullptr};
};

#endif // MAINWINDOW_H
