#include "PhysicsCanvas.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QUrl>
#include <QtQml>
#ifdef ORES_PHYSICS_GUI_PROOF
void start_gui_proof(QQmlApplicationEngine& engine);
#endif

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<PhysicsCanvas>("OresPhysics", 1, 0, "PhysicsCanvas");
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/ui/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 2;
#ifdef ORES_PHYSICS_GUI_PROOF
    if (qEnvironmentVariableIsSet("ATTO_PHYSICS_GUI_PROOF")) start_gui_proof(engine);
#endif
    return app.exec();
}
