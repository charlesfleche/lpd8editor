#include "application.h"
#include "db.h"
#include "mainwindow.h"
#include "utils.h"

#include <QApplication>
#include <QMainWindow>

#include <QtDebug>

int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x050600
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    app.setOrganizationName("lpd8-editor");
    app.setApplicationName("lpd8-editor");
    app.setApplicationVersion("0.0.0");

    if (!initFilesystem()) {
        throw std::runtime_error("Failed filesystem initialization");
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::runtime_error("Failed database initialization");
    }

    Application application;
    MainWindow win(&application);
    win.show();
    return app.exec();
}
