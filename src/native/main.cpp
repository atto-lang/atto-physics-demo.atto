#include "PhysicsCanvas.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QUrl>
#include <QDebug>
#include <QPointer>
#include <QTimer>
#include <QtQml>

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<PhysicsCanvas>("OresPhysics", 1, 0, "PhysicsCanvas");
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/ui/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 2;
    // Offscreen integration test: QML must instantiate the real canvas, its
    // timer must consume a real physics snapshot, and the worker must advance.
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        QPointer<PhysicsCanvas> canvas(
            engine.rootObjects().constFirst()->findChild<PhysicsCanvas*>(
                QStringLiteral("physicsCanvas")));
        if (!canvas) return 3;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, &app, [&app, canvas] {
            if (canvas && canvas->tick() > 0) {
                qInfo() << "ATTO_QT_QML_LIVE_GUEST_SMOKE_PASS" << canvas->tick();
                app.exit(0);
            }
        });
        poll.start(25);
        QTimer::singleShot(5000, &app, [&app] {
            qCritical() << "PhysicsCanvas did not observe a live guest tick";
            app.exit(4);
        });
        return app.exec();
    }
    return app.exec();
}
