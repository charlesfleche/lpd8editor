#include "mainwindow.h"
#include "app_info.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QStandardPaths>

#include <QtDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName(appName);
    app.setApplicationName(appName);
    app.setApplicationVersion(appVersion);

    app.setProperty("applicationLink", appUrl);
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    auto shareDir = QDir(app.applicationDirPath());
    shareDir.cdUp();
    shareDir.cd("share");
    shareDir.cd(app.organizationName());
    shareDir.cd(app.applicationName());
    QStringList paths = {
        app.applicationDirPath(),  // Useful for testing from the build folder
        shareDir.path()            // Useful for installations in a non-standard location
    };
    paths += QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    MainWindow win;
    win.setWindowTitle(app.applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    return app.exec();
}
