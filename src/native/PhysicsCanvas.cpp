#include "PhysicsCanvas.hpp"

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QtGlobal>

PhysicsCanvas::PhysicsCanvas(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setOpaquePainting(true);
    engine_.start();
}

void PhysicsCanvas::setPaused(bool v) {
    if (paused() == v) return;
    engine_.set_paused(v);
    emit pausedChanged();
}

void PhysicsCanvas::reset() { engine_.request_reset(); }

void PhysicsCanvas::refresh() {
    if (!engine_.read_latest(snapshot_)) return;
    emit tickChanged();
    update();
}

void PhysicsCanvas::paint(QPainter* painter) {
    painter->fillRect(boundingRect(), QColor("#0d1626"));
    painter->save();
    painter->scale(width() / 960.0, height() / 560.0);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(QColor("#283b55"), 1.0));
    for (int x = 0; x <= 960; x += 48)
        painter->drawLine(x, 0, x, 560);
    for (int y = 0; y <= 560; y += 40)
        painter->drawLine(0, y, 960, y);
    painter->setPen(QPen(QColor("#91a9c6"), 2.0));
    painter->drawRect(QRectF(1, 1, 958, 558));
    painter->setPen(Qt::NoPen);
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        const qreal x = static_cast<qreal>(snapshot_.x[i]) / ores::kScale;
        const qreal y = static_cast<qreal>(snapshot_.y[i]) / ores::kScale;
        const qreal r = static_cast<qreal>(snapshot_.radius[i]) / ores::kScale;
        painter->setBrush(QColor::fromHsv((static_cast<int>(i) * 37) % 360, 160, 255));
        painter->drawEllipse(QPointF(x, y), r, r);
    }
    painter->restore();
}
