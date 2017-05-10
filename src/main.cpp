#include "application.h"
#include "mainwindow.h"

#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
//    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setOrganizationName("lpd8-editor");
    app.setApplicationName("lpd8-editor");
    app.setApplicationVersion("0.0.0");

    Application application;

    MainWindow win(&application);
    win.show();

//    Application app;

    return app.exec();
}
