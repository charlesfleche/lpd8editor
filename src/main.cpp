#include "app_info.h"
#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setOrganizationName(appName);
    QApplication::setApplicationName(appName);
    QApplication::setApplicationVersion(appVersion);

    app.setProperty("applicationLink", appUrl);
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    MainWindow win;
    win.setWindowTitle(QApplication::applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    return QApplication::exec();
}
