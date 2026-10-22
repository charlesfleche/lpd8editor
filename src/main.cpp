#include "app_info.h"
#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QStandardPaths>

#include <QtDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setOrganizationName(appName);
    QApplication::setApplicationName(appName);
    QApplication::setApplicationVersion(appVersion);

    app.setProperty("applicationLink", appUrl);
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    auto shareDir = QDir(QApplication::applicationDirPath());
    shareDir.cdUp();
    shareDir.cd("share");
    shareDir.cd(QApplication::organizationName());
    shareDir.cd(QApplication::applicationName());
    QStringList paths = {
            QApplication::applicationDirPath(), // Useful for testing from the build folder
            shareDir.path() // Useful for installations in a non-standard location
    };
    paths += QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    MainWindow win;
    win.setWindowTitle(QApplication::applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    return QApplication::exec();
}
