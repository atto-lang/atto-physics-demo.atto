#pragma once

#include "SnapshotRing.hpp"
#include <atomic>
#include <cstdint>
#include <thread>

namespace ores {
// start()/stop() belong to a single controller thread.
// The simulation worker produces snapshots; only the GUI consumes them.
class PhysicsEngine final {
public:
    PhysicsEngine() = default;
    ~PhysicsEngine();
    PhysicsEngine(const PhysicsEngine&) = delete;
    PhysicsEngine& operator=(const PhysicsEngine&) = delete;

    void start();
    void stop() noexcept;
    void request_reset() noexcept { reset_requested_.store(true, std::memory_order_release); }
    void set_paused(bool v) noexcept { paused_.store(v, std::memory_order_release); }
    bool paused() const noexcept { return paused_.load(std::memory_order_acquire); }
    bool read_latest(Snapshot& out) noexcept { return ring_.consume_latest(out); }
    std::uint64_t dropped_snapshots() const noexcept { return ring_.skipped_snapshots(); }

private:
    void run(std::uint64_t generation) noexcept;
    void publish(const World& world, Snapshot& scratch, std::uint64_t late,
                 std::uint64_t generation) noexcept;

    SnapshotRing ring_{};
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> reset_requested_{false};
    // Controller-owned; not read from the simulation thread directly.
    std::uint64_t next_generation_ = 0;
    std::thread worker_{};
};
}  // namespace ores
