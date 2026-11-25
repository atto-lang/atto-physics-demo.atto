#pragma once

#include "PhysicsCore.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

namespace ores {
// Single-producer/single-consumer. The producer never waits for rendering.
// The consumer retains ownership of unread slots until the copy completes.
class SnapshotRing final {
public:
    static constexpr std::uint32_t kCapacity = 4;
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "Lock-free indices required for the simulation boundary");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "Lock-free telemetry required for the simulation boundary");
    static_assert(std::is_trivially_copyable_v<Snapshot>);

    bool publish(const Snapshot& snapshot) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if (static_cast<std::uint32_t>(head - tail) >= kCapacity - 1) {
            skipped_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        slots_[head % kCapacity] = snapshot;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool consume_latest(Snapshot& out) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        if (tail == head) return false;
        // Producer cannot overwrite this unread slot until tail advances.
        out = slots_[(head - 1) % kCapacity];
        skipped_.fetch_add(static_cast<std::uint32_t>(head - tail - 1),
                           std::memory_order_relaxed);
        tail_.store(head, std::memory_order_release);
        return true;
    }

    std::uint64_t skipped_snapshots() const noexcept {
        return skipped_.load(std::memory_order_relaxed);
    }

private:
    std::array<Snapshot, kCapacity> slots_{};
    alignas(64) std::atomic<std::uint32_t> head_{0};
    alignas(64) std::atomic<std::uint32_t> tail_{0};
    std::atomic<std::uint64_t> skipped_{0};
};
}  // namespace ores
