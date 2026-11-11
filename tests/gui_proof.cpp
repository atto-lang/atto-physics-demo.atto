// Opt-in integration harness: actual window, rendered frames and mouse events.
// No physics implementation lives here.
#include "PhysicsCanvas.hpp"
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTest>
#include <QTimer>
#include <QDir>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {
struct Proof {
    QQuickWindow* window;
    PhysicsCanvas* field;
    QString directory;
    int stage = 0;
    qulonglong tick = 0;
    QImage previous;
    QImage initial;
    void check(bool ok, const char* why) {
        if (!ok) { std::fprintf(stderr, "GUI_PROOF_FAIL: %s\n", why); std::exit(3); }
    }
    QImage frame(const char* name) {
        auto full = window->grabWindow();
        check(!full.isNull(), "visible window capture");
        check(full.save(directory + "/" + name + ".png"), "capture saved");
        auto origin = field->mapToScene(QPointF(0, 0));
        auto ratio = window->devicePixelRatio();
        return full.copy(QRect(origin.x()*ratio, origin.y()*ratio,
                               field->width()*ratio, field->height()*ratio));
    }
    void click(const char* name) {
        auto* button = window->findChild<QQuickItem*>(name);
        check(button != nullptr, "QML button exists");
        auto point = button->mapToScene(QPointF(button->width()/2, button->height()/2));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point.toPoint());
    }
    void advance() {
        switch (stage++) {
        case 0:
            check(window->isVisible() && field->tick() > 0, "simulation started");
            tick = field->tick(); previous = frame("01-running"); break;
        case 1:
            check(field->tick() > tick, "simulation tick advanced");
            check(frame("02-moving") != previous, "body pixels moved");
            click("pauseButton"); check(field->paused(), "Pause mouse click"); break;
        case 2:
            tick = field->tick(); previous = frame("03-paused"); break;
        case 3:
            check(field->tick() == tick, "paused tick stable");
            check(frame("04-still-paused") == previous, "paused body pixels stable");
            click("resetButton"); break;
        case 4:
            check(field->tick() == 0 && field->paused(), "Reset while paused");
            initial = frame("05-reset");
            check(initial != previous, "reset changed body pixels");
            click("pauseButton"); check(!field->paused(), "Resume mouse click"); break;
        case 5:
            check(field->tick() > 0, "resume stepped guest");
            check(frame("06-resumed") != initial, "resumed body pixels moved");
            click("pauseButton"); click("resetButton"); break;
        case 6:
            check(field->tick() == 0, "second reset");
            check(frame("07-reset-again") == initial, "deterministic rendered reset");
            std::puts("PACKAGED_QT_MOUSE_RENDER_GUEST_PROOF_PASS");
            QCoreApplication::quit(); break;
        }
    }
};
}
void start_gui_proof(QQmlApplicationEngine& engine) {
    auto state = std::make_shared<Proof>();
    state->window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    state->check(state->window != nullptr, "QQuickWindow exists");
    state->field = state->window->findChild<PhysicsCanvas*>("physicsField");
    state->check(state->field != nullptr, "live canvas exists");
    state->directory = qEnvironmentVariable("ATTO_PHYSICS_GUI_PROOF");
    state->check(QDir().mkpath(state->directory), "proof directory");
    auto* timer = new QTimer(state->window);
    QObject::connect(timer, &QTimer::timeout, state->window, [state] { state->advance(); });
    timer->start(500);
}
