#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x050600
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    app.setOrganizationName("lpd8-editor");
    app.setApplicationName("lpd8-editor");
    app.setApplicationVersion("0.0.7");

    app.setProperty("applicationLink", "https://github.com/charlesfleche/lpd8-editor");
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    MainWindow win;
    win.setWindowTitle(app.applicationName());
    win.setWindowIcon(QIcon(":/icon.svg"));
    win.show();
    return app.exec();
}
