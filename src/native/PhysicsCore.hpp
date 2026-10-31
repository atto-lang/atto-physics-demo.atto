#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace ores {
inline constexpr std::size_t kBodies = 192;
inline constexpr std::int64_t kScale = 256;
inline constexpr std::int64_t kWidth = 960 * kScale;
inline constexpr std::int64_t kHeight = 560 * kScale;

struct World final {
    std::array<std::int64_t, kBodies> x{};
    std::array<std::int64_t, kBodies> y{};
    std::array<std::int64_t, kBodies> vx{};
    std::array<std::int64_t, kBodies> vy{};
    std::array<std::int64_t, kBodies> radius{};
    std::uint64_t tick = 0;
};

struct Snapshot final {
    std::array<std::int64_t, kBodies> x{};
    std::array<std::int64_t, kBodies> y{};
    std::array<std::int64_t, kBodies> radius{};
    std::uint64_t tick = 0;
    std::uint64_t missed_deadlines = 0;
};

void reset_world(World& state) noexcept;
void step_world(World& state) noexcept;
void copy_snapshot(const World& state, Snapshot& snapshot, std::uint64_t late) noexcept;
}  // namespace ores
