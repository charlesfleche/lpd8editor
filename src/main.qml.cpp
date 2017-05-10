/*
#include "application.h"
#include "qmltablemodelproxy.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication application(argc, argv);
    application.setOrganizationName("lpd8-editor");
    application.setApplicationName("lpd8-editor");
    application.setApplicationVersion("0.0.0");

    Application app;

    QmlTableModelProxy programs;
    programs.setSourceModel(app.programs());

    QmlTableModelProxy pads;
    pads.setSourceModel(app.pads());

    QmlTableModelProxy knobs;
    knobs.setSourceModel(app.knobs());

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("app", &app);
    engine.rootContext()->setContextProperty("programsModel", &programs);
    engine.rootContext()->setContextProperty("padsModel", &pads);
    engine.rootContext()->setContextProperty("knobsModel", &knobs);
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));

    return application.exec();
}
*/
