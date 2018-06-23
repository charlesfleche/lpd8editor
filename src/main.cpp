#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QStandardPaths>
#include <QTranslator>

#include <QtDebug>

const QString app_name = QString("lpd8editor");

int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x050600
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    app.setOrganizationName(app_name);
    app.setApplicationName(app_name);
    app.setApplicationVersion("0.0.12");

    app.setProperty("applicationLink", "https://github.com/charlesfleche/lpd8editor");
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    QTranslator translator;
    const QLocale locale = QLocale::system();
    const QStringList paths = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (auto it = paths.constBegin() ; it != paths.constEnd() ; ++it) {
        if (translator.load(locale, app_name, ".", *it)) {
            qDebug() << "Loaded translation for" << locale << "from" << *it;
            app.installTranslator(&translator);
            break;
        }
    }

    MainWindow win;
    win.setWindowTitle(app.applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    return app.exec();
}
