#include "PhysicsEngine.hpp"

#include <chrono>
#include <thread>

namespace ores {
PhysicsEngine::~PhysicsEngine() { stop(); }

void PhysicsEngine::start() {
    if (running_.exchange(true, std::memory_order_acq_rel)) return;
    try {
        worker_ = std::thread([this] { run(); });
    } catch (...) {
        running_.store(false, std::memory_order_release);
        throw;
    }
}

void PhysicsEngine::stop() noexcept {
    running_.store(false, std::memory_order_release);
    if (worker_.joinable()) worker_.join();
}

void PhysicsEngine::publish(const World& world, std::uint64_t late) noexcept {
    const auto head = head_.load(std::memory_order_relaxed);
    const auto tail = tail_.load(std::memory_order_acquire);
    if (head - tail >= kCapacity - 1) {
        dropped_.fetch_add(1, std::memory_order_relaxed);
        return;  // Never wait for the QML/render thread.
    }
    copy_snapshot(world, ring_[head % kCapacity], late);
    head_.store(head + 1, std::memory_order_release);
}

bool PhysicsEngine::read_latest(Snapshot& out) noexcept {
    auto tail = tail_.load(std::memory_order_relaxed);
    const auto head = head_.load(std::memory_order_acquire);
    if (tail == head) return false;
    while (tail != head) {
        out = ring_[tail % kCapacity];
        ++tail;
    }
    tail_.store(tail, std::memory_order_release);
    return true;
}

void PhysicsEngine::run() noexcept {
    using clock = std::chrono::steady_clock;
    constexpr auto period = std::chrono::nanoseconds(16'666'667);
    World world{};
    reset_world(world);  // Initialization is explicitly outside steady-state ticks.
    publish(world, 0);
    std::uint64_t late = 0;
    auto deadline = clock::now();
    while (running_.load(std::memory_order_acquire)) {
        deadline += period;
        std::this_thread::sleep_until(deadline);
        if (!running_.load(std::memory_order_acquire)) break;
        const auto now = clock::now();
        if (now > deadline) {
            // Record a *missed frame*, not mere positive wakeup jitter.
            if (now - deadline >= period) {
                ++late;
                deadline = now;  // Skip catch-up bursts.
            }
        }
        if (reset_requested_.exchange(false, std::memory_order_acq_rel))
            reset_world(world);
        if (!paused_.load(std::memory_order_acquire)) step_world(world);
        publish(world, late);
    }
}
}  // namespace ores
