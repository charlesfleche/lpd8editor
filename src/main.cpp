#include "mainwindow.h"
#include "app_info.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QStandardPaths>
#include <QTranslator>

#include <QtDebug>

int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x050600
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
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

    QTranslator translator;
    const QLocale locale = QLocale::system();

    for (auto it = paths.constBegin() ; it != paths.constEnd() ; ++it) {
        qDebug() << "Looking for translations" << locale << "in folder" << *it;
        if (translator.load(locale, appName, ".", *it)) {
            app.installTranslator(&translator);
            qDebug() << "Loaded translation" << locale << "from" << *it;
            break;
        }
    }

    MainWindow win;
    win.setWindowTitle(app.applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    return app.exec();
}
