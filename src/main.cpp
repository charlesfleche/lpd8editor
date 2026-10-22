#include "app_info.h"
#include "db.h"
#include "mainwindow.h"
#include "utils.h"

#include <QApplication>
#include <QIcon>
#include <QMessageBox>

#include <cstdlib>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setOrganizationName(appName);
    QApplication::setApplicationName(appName);
    QApplication::setApplicationVersion(appVersion);

    app.setProperty("applicationLink", appUrl);
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    // Open the database before creating the window: if it fails, the user is
    // told why, instead of the program dying on an uncaught exception.
    const std::expected<QSqlDatabase, QString> db = openDatabase(defaultDbPath());
    if (!db) {
        QMessageBox::critical(nullptr, QApplication::applicationName(), db.error());
        return EXIT_FAILURE;
    }

    MainWindow win(*db);
    win.setWindowTitle(QApplication::applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    return QApplication::exec();
}
