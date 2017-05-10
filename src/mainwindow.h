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

private:
    int programModelColumn() const;

    Ui::MainWindow *ui;
    Application* app;
};

#endif // MAINWINDOW_H
