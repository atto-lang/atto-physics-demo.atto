#pragma once

#include "PhysicsCore.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <thread>

namespace ores {
// Single-producer/single-consumer bounded queue, no locks on the producer.
// The physics thread is the only writer of World and ring slots; the QML
// thread is the only consumer. Release/acquire publishes completed snapshots.
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
    bool read_latest(Snapshot& out) noexcept;
    std::uint64_t dropped_snapshots() const noexcept {
        return dropped_.load(std::memory_order_relaxed);
    }

private:
    static constexpr std::uint32_t kCapacity = 4;
    void run() noexcept;
    void publish(const World& world, std::uint64_t late) noexcept;

    std::array<Snapshot, kCapacity> ring_{};
    std::atomic<std::uint32_t> head_{0};
    std::atomic<std::uint32_t> tail_{0};
    std::atomic<std::uint64_t> dropped_{0};
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> reset_requested_{false};
    std::thread worker_{};
};
}  // namespace ores
