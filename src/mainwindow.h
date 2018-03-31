#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class Application;

class QPushButton;
class QUndoStack;

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

    void refreshUiAccordingToSelection();
    void refreshWidgetStack();

private:
    int currentSelectedProjectId() const;
    int programModelColumn() const;

    Ui::MainWindow *ui;
    Application* app;
    QList<QPushButton*> midiChannelButtons;
};

#endif // MAINWINDOW_H
