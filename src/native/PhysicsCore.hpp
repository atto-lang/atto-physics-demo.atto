#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ores {
inline constexpr std::size_t kBodies = 192;
inline constexpr std::int64_t kScale = 256;
inline constexpr std::int64_t kWidth = 960 * kScale;
inline constexpr std::int64_t kHeight = 560 * kScale;
// Fixed, closed input envelope for reset_world + step_world ONLY.
// Exported guest functions themselves do not validate arbitrary FFI inputs.
inline constexpr std::int64_t kMinRadius = 7 * kScale;
inline constexpr std::int64_t kMaxRadius = 13 * kScale;
inline constexpr std::int64_t kMaxHorizontalSpeed = 4 * 97;
inline constexpr std::int64_t kGravityPerTick = 42;
inline constexpr std::int64_t kTerminalVerticalSpeed = 7 * kScale;
inline constexpr std::int64_t kMaxInitialVerticalSpeed = 10 * 51 + 75;
inline constexpr std::int64_t kMaxVerticalSpeed =
    kTerminalVerticalSpeed > kMaxInitialVerticalSpeed
        ? kTerminalVerticalSpeed : kMaxInitialVerticalSpeed;

// Positions stay in [radius, extent-radius] after every advance.
// If |velocity| <= minimum radius, the pre-clamp sum stays in [0,extent].
// The gravity clamp and reflection preserve these velocity envelopes.
static_assert(kMinRadius > 0 && kMaxRadius < kHeight / 2);
static_assert(kMaxRadius < kWidth / 2);
static_assert(kMaxHorizontalSpeed <= kMinRadius);
static_assert(kMaxVerticalSpeed <= kMinRadius);
static_assert(kGravityPerTick > 0 && kTerminalVerticalSpeed >= kMaxInitialVerticalSpeed);
static_assert(kWidth < std::numeric_limits<std::int64_t>::max() - kMaxHorizontalSpeed);
static_assert(kHeight < std::numeric_limits<std::int64_t>::max() - kMaxVerticalSpeed);

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
