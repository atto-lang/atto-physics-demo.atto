#pragma once

#include "PhysicsCore.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ores {
// Presentation/transport layout experiment only. All physics arithmetic and
// authoritative World state remain the real Oreslang-powered SoA world.
// Group each 16-body slice into a cache-aligned block; no malloc/new.
inline constexpr std::size_t kAoSoaLanes = 16;
static_assert(kBodies % kAoSoaLanes == 0);

struct alignas(64) AoSoaSnapshotBlock final {
    std::array<std::int64_t, kAoSoaLanes> x{};
    std::array<std::int64_t, kAoSoaLanes> y{};
    std::array<std::int64_t, kAoSoaLanes> radius{};
};
struct AoSoaSnapshot final {
    std::array<AoSoaSnapshotBlock, kBodies / kAoSoaLanes> blocks{};
    std::uint64_t tick = 0;
    std::uint64_t generation = 0;
    std::uint64_t missed_deadlines = 0;
};
static_assert(std::is_trivially_copyable_v<AoSoaSnapshot>);
static_assert(alignof(AoSoaSnapshotBlock) == 64);

// Both functions are bounded, allocation-free and do not mutate the input.
inline void pack_snapshot_aosoa(const Snapshot& source, AoSoaSnapshot& dest) noexcept {
    for (std::size_t block = 0; block < dest.blocks.size(); ++block) {
        for (std::size_t lane = 0; lane < kAoSoaLanes; ++lane) {
            const auto i = block * kAoSoaLanes + lane;
            dest.blocks[block].x[lane] = source.x[i];
            dest.blocks[block].y[lane] = source.y[i];
            dest.blocks[block].radius[lane] = source.radius[i];
        }
    }
    dest.tick = source.tick;
    dest.generation = source.generation;
    dest.missed_deadlines = source.missed_deadlines;
}
inline void unpack_snapshot_aosoa(const AoSoaSnapshot& source, Snapshot& dest) noexcept {
    for (std::size_t block = 0; block < source.blocks.size(); ++block) {
        for (std::size_t lane = 0; lane < kAoSoaLanes; ++lane) {
            const auto i = block * kAoSoaLanes + lane;
            dest.x[i] = source.blocks[block].x[lane];
            dest.y[i] = source.blocks[block].y[lane];
            dest.radius[i] = source.blocks[block].radius[lane];
        }
    }
    dest.tick = source.tick;
    dest.generation = source.generation;
    dest.missed_deadlines = source.missed_deadlines;
}
}  // namespace ores
