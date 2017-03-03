#include "application.h"
#include "qmltablemodelproxy.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication application(argc, argv);
    application.setApplicationName("lpd8-editor");
    application.setApplicationVersion("0.0.0");

    Application app;

    QmlTableModelProxy presets;
    presets.setSourceModel(app.presets());

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("presets", &presets);
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));

    return application.exec();
}
