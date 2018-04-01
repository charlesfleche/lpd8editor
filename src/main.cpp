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
    app.setApplicationVersion("0.0.0");

    MainWindow win;
    win.show();
    return app.exec();
}
