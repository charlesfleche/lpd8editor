#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class Application;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Application*, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionNewProgram_triggered();
    void on_actionDeleteProgram_triggered();

    void on_programsView_activated(const QModelIndex&);

    void refreshActionDeleteProgram();

private:
    int programModelColumn() const;

    Ui::MainWindow *ui;
    Application* app;
};

#endif // MAINWINDOW_H
