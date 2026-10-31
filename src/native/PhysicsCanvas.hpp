#pragma once

#include "PhysicsEngine.hpp"
#include <QQuickPaintedItem>
#include <mutex>

class PhysicsCanvas final : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(qulonglong tick READ tick NOTIFY tickChanged)
    Q_PROPERTY(qulonglong missedDeadlines READ missedDeadlines NOTIFY tickChanged)
    Q_PROPERTY(qulonglong droppedSnapshots READ droppedSnapshots NOTIFY tickChanged)
public:
    explicit PhysicsCanvas(QQuickItem* parent = nullptr);
    ~PhysicsCanvas() override;
    void paint(QPainter* painter) override;
    bool paused() const noexcept { return engine_.paused(); }
    void setPaused(bool v);
    qulonglong tick() const;
    qulonglong missedDeadlines() const;
    qulonglong droppedSnapshots() const noexcept { return engine_.dropped_snapshots(); }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void reset();

signals:
    void pausedChanged();
    void tickChanged();

private:
    ores::PhysicsEngine engine_{};
    mutable std::mutex snapshot_mutex_{};
    ores::Snapshot snapshot_{};
};
