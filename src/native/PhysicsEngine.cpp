#include "PhysicsEngine.hpp"

#include <chrono>
#include <thread>
#include <limits>
#include <stdexcept>

namespace ores {
PhysicsEngine::~PhysicsEngine() { stop(); }

void PhysicsEngine::start() {
    if (running_.exchange(true, std::memory_order_acq_rel)) return;
    // start()/stop()/read_latest() are controlled by one GUI thread. No
    // producer exists here and the consumer is quiescent. A new worker must
    // not publish alongside unread frames from the previous generation.
    ring_.clear_quiescent();
    if (next_generation_ == std::numeric_limits<std::uint64_t>::max()) {
        running_.store(false, std::memory_order_release);
        throw std::overflow_error("PhysicsEngine generation exhausted");
    }
    const auto generation = ++next_generation_;
    try {
        worker_ = std::thread([this, generation] { run(generation); });
    } catch (...) {
        running_.store(false, std::memory_order_release);
        throw;
    }
}

void PhysicsEngine::stop() noexcept {
    running_.store(false, std::memory_order_release);
    if (worker_.joinable()) worker_.join();
}

void PhysicsEngine::publish(const World& world, Snapshot& scratch,
                            std::uint64_t late, std::uint64_t generation) noexcept {
    copy_snapshot(world, scratch, late);
    scratch.generation = generation;
    (void)ring_.publish(scratch);  // Never wait for the QML thread.
}

void PhysicsEngine::run(std::uint64_t generation) noexcept {
    using clock = std::chrono::steady_clock;
    constexpr auto period = std::chrono::nanoseconds(16'666'667);
    World world{};
    Snapshot scratch{};  // Initialized once, outside the steady-state loop.
    reset_world(world);
    publish(world, scratch, 0, generation);
    std::uint64_t late = 0;
    auto deadline = clock::now();
    while (running_.load(std::memory_order_acquire)) {
        deadline += period;
        std::this_thread::sleep_until(deadline);
        if (!running_.load(std::memory_order_acquire)) break;
        const auto now = clock::now();
        if (now > deadline && now - deadline >= period) {
            ++late;
            deadline = now;  // No catch-up bursts.
        }
        if (reset_requested_.exchange(false, std::memory_order_acq_rel))
            reset_world(world);
        if (!paused_.load(std::memory_order_acquire)) step_world(world);
        publish(world, scratch, late, generation);
    }
}
}  // namespace ores
