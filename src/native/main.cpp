#include "PhysicsCanvas.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QUrl>
#include <QtQml>

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<PhysicsCanvas>("OresPhysics", 1, 0, "PhysicsCanvas");
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/ui/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 2;
    return app.exec();
}
