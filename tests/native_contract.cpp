#include "PhysicsCore.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

extern "C" {
std::int64_t physics_advance_axis(std::int64_t, std::int64_t, std::int64_t, std::int64_t);
std::int64_t physics_reflect_axis(std::int64_t, std::int64_t, std::int64_t, std::int64_t);
std::int64_t physics_gravity(std::int64_t, std::int64_t, std::int64_t);
}
#define CHECK(cond) do { if (!(cond)) { \
  std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
  std::abort(); \
} } while (false)

static std::uint64_t digest(const ores::World& world) {
    std::uint64_t h = 1469598103934665603ULL;
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        for (auto v : {world.x[i], world.y[i], world.vx[i], world.vy[i]}) {
            h ^= static_cast<std::uint64_t>(v);
            h *= 1099511628211ULL;
        }
    }
    return h;
}

static void check_world_envelope(const ores::World& world) {
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        const auto r = world.radius[i];
        CHECK(r >= ores::kMinRadius && r <= ores::kMaxRadius);
        CHECK(world.x[i] >= r && world.x[i] <= ores::kWidth - r);
        CHECK(world.y[i] >= r && world.y[i] <= ores::kHeight - r);
        CHECK(world.vx[i] >= -ores::kMaxHorizontalSpeed);
        CHECK(world.vx[i] <= ores::kMaxHorizontalSpeed);
        CHECK(world.vy[i] >= -ores::kMaxVerticalSpeed);
        CHECK(world.vy[i] <= ores::kMaxVerticalSpeed);
    }
}

int main() {
    // The called functions must be supplied by actual compiler-generated .ores
    // object code; there is deliberately no C++ physics substitute.
    CHECK(physics_advance_axis(10, 5, 0, 100) == 15);
    CHECK(physics_advance_axis(98, 5, 0, 100) == 100);
    CHECK(physics_advance_axis(2, -5, 0, 100) == 0);
    CHECK(physics_advance_axis(100, 0, 0, 100) == 100);
    CHECK(physics_advance_axis(0, 0, 0, 100) == 0);
    CHECK(physics_reflect_axis(98, 5, 0, 100) == -5);
    CHECK(physics_reflect_axis(2, -5, 0, 100) == 5);
    CHECK(physics_reflect_axis(50, 5, 0, 100) == 5);
    CHECK(physics_reflect_axis(100, 5, 0, 100) == -5);
    CHECK(physics_reflect_axis(0, -5, 0, 100) == 5);
    CHECK(physics_reflect_axis(100, 0, 0, 100) == 0);
    CHECK(physics_gravity(-3, 2, 10) == -1);
    CHECK(physics_gravity(9, 3, 10) == 10);
    CHECK(physics_gravity(-ores::kMaxVerticalSpeed, ores::kGravityPerTick,
                          ores::kTerminalVerticalSpeed) ==
          -ores::kMaxVerticalSpeed + ores::kGravityPerTick);
    CHECK(physics_gravity(ores::kTerminalVerticalSpeed, ores::kGravityPerTick,
                          ores::kTerminalVerticalSpeed) == ores::kTerminalVerticalSpeed);
    ores::World a{}, b{};
    ores::reset_world(a);
    ores::reset_world(b);
    CHECK(digest(a) == digest(b));
    check_world_envelope(a);
    ores::step_world(a);
    // Independent fixed-point oracle, checking actual linked .ores execution.
    CHECK(a.x[0] == 36 * ores::kScale - 4 * 97);
    CHECK(a.y[0] == 28 * ores::kScale - 75 + ores::kGravityPerTick);
    CHECK(a.vx[0] == -4 * 97);
    CHECK(a.vy[0] == -75 + ores::kGravityPerTick);
    ores::reset_world(a);
    for (std::size_t n = 0; n < 1200; ++n) {
        ores::step_world(a);
        ores::step_world(b);
        CHECK(digest(a) == digest(b));
        CHECK(a.tick == n + 1 && b.tick == n + 1);
        check_world_envelope(a);
    }
    const auto first_run_digest = digest(a);
    ores::reset_world(a);
    CHECK(a.tick == 0);
    check_world_envelope(a);
    for (std::size_t n = 0; n < 1200; ++n) {
        ores::step_world(a);
    }
    CHECK(digest(a) == first_run_digest);
    CHECK(a.tick == 1200 && b.tick == 1200);
    CHECK(digest(a) != 0);
    std::printf("PHYSICS_GUEST_1200_TICK_DIGEST=%016llx\n",
                static_cast<unsigned long long>(first_run_digest));
    std::puts("REAL_ORESLANG_LLVM_PHYSICS_CONTRACT_PASS");
}
