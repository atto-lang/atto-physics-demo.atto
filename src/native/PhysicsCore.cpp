#include "PhysicsCore.hpp"

#include <cstdint>

// These are REAL Oreslang functions: oreslang-llvmc -> LLVM IR -> Clang object.
// Do not replace them with host-language physics implementations.
extern "C" {
std::int64_t physics_initial_radius(std::int64_t);
std::int64_t physics_initial_x(std::int64_t);
std::int64_t physics_initial_y(std::int64_t);
std::int64_t physics_initial_vx(std::int64_t);
std::int64_t physics_initial_vy(std::int64_t);
std::int64_t physics_advance_axis(std::int64_t position, std::int64_t velocity,
                                  std::int64_t lower, std::int64_t upper);
std::int64_t physics_reflect_axis(std::int64_t position, std::int64_t velocity,
                                  std::int64_t lower, std::int64_t upper);
std::int64_t physics_gravity(std::int64_t velocity, std::int64_t acceleration,
                             std::int64_t terminal);
}

namespace ores {
void reset_world(World& w) noexcept {
    w = World{};
    // Fixed initial positions, a structure of five preallocated scalar columns.
    // Nothing is allocated in step_world.
    for (std::size_t i = 0; i < kBodies; ++i) {
        const auto index = static_cast<std::int64_t>(i);
        w.radius[i] = physics_initial_radius(index);
        w.x[i] = physics_initial_x(index);
        w.y[i] = physics_initial_y(index);
        w.vx[i] = physics_initial_vx(index);
        w.vy[i] = physics_initial_vy(index);
    }
}

void step_world(World& w) noexcept {
    // Bounds and loop cardinality are fixed; all trajectory math is Oreslang.
    // This v1 does not implement body-to-body collisions or parallel tasks.
    for (std::size_t i = 0; i < kBodies; ++i) {
        const std::int64_t r = w.radius[i];
        const std::int64_t next_vy = physics_gravity(w.vy[i], kGravityPerTick, kTerminalVerticalSpeed);
        const std::int64_t next_vx = physics_reflect_axis(w.x[i], w.vx[i], r, kWidth - r);
        const std::int64_t next_vy_bounced =
            physics_reflect_axis(w.y[i], next_vy, r, kHeight - r);
        const std::int64_t next_x = physics_advance_axis(w.x[i], w.vx[i], r, kWidth - r);
        const std::int64_t next_y = physics_advance_axis(w.y[i], next_vy, r, kHeight - r);
        w.x[i] = next_x;
        w.y[i] = next_y;
        w.vx[i] = next_vx;
        w.vy[i] = next_vy_bounced;
    }
    ++w.tick;
}

void copy_snapshot(const World& w, Snapshot& out, std::uint64_t late) noexcept {
    out.x = w.x;
    out.y = w.y;
    out.radius = w.radius;
    out.tick = w.tick;
    out.generation = 0;  // Bare copy is unowned until engine stamps its epoch.
    out.missed_deadlines = late;
}
}  // namespace ores
