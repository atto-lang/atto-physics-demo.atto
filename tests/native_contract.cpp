#include "PhysicsCore.hpp"
#include "SnapshotLayouts.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

extern "C" {
std::int64_t physics_advance_axis(std::int64_t, std::int64_t, std::int64_t, std::int64_t);
std::int64_t physics_reflect_axis(std::int64_t, std::int64_t, std::int64_t, std::int64_t);
std::int64_t physics_gravity(std::int64_t, std::int64_t, std::int64_t);
std::int64_t physics_discs_overlap(std::int64_t, std::int64_t, std::int64_t,
                                   std::int64_t, std::int64_t, std::int64_t);
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
    // Compiler-generated Oreslang narrowphase, NOT a host stand-in.
    // Strict tangency is not penetration; invalid FFI inputs fail closed.
    CHECK(physics_discs_overlap(10000, 10000, 2000, 13999, 10000, 2000) == 1);
    CHECK(physics_discs_overlap(10000, 10000, 2000, 14000, 10000, 2000) == 0);
    CHECK(physics_discs_overlap(10000, 10000, 2000, 14001, 10000, 2000) == 0);
    CHECK(physics_discs_overlap(10000, 10000, 2000, 10000, 10000, 2000) == 1);
    CHECK(physics_discs_overlap(10000, 10000, 2000, 12828, 12828, 2000) == 1);
    CHECK(physics_discs_overlap(10000, 10000, 2000, 12829, 12829, 2000) == 0);
    CHECK(physics_discs_overlap(0, 10000, 2000, 10000, 10000, 2000) == -1);
    CHECK(physics_discs_overlap(10000, 10000, -1, 12000, 10000, 2000) == -1);
    CHECK(physics_discs_overlap(10000, 10000, 5000, 12000, 10000, 2000) == -1);
    CHECK(physics_discs_overlap(10000, 10000, 2000, 245760, 10000, 2000) == -1);
    CHECK(physics_discs_overlap(std::numeric_limits<std::int64_t>::min(),
                               10000, 2000, 10000, 10000, 2000) == -1);
    CHECK(physics_discs_overlap(10000, 10000,
                               std::numeric_limits<std::int64_t>::max(),
                               10000, 10000, 2000) == -1);
    for (std::int64_t n = 0; n < 96; ++n) {
        const auto ax = 10000 + n * 31;
        const auto ay = 10000 + n * 17;
        const auto bx = 15000 + n * 23;
        const auto by = 12000 + n * 29;
        const auto lr = 1792 + n % 20;
        const auto rr = 1800 + n % 11;
        const auto forward = physics_discs_overlap(ax, ay, lr, bx, by, rr);
        const auto reverse = physics_discs_overlap(bx, by, rr, ax, ay, lr);
        CHECK(forward == reverse);
        CHECK(forward == 0 || forward == 1);
    }
    ores::World a{}, b{};
    ores::reset_world(a);
    ores::reset_world(b);
    CHECK(digest(a) == digest(b));
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        // Independent initialization oracle for compiled guest functions.
        CHECK(a.radius[i] == (7 + static_cast<std::int64_t>(i % 7)) * 256);
        CHECK(a.x[i] == (36 + static_cast<std::int64_t>(i % 16) * 57) * 256);
        CHECK(a.y[i] == (28 + static_cast<std::int64_t>(i / 16) * 36) * 256);
        CHECK(a.vx[i] == (static_cast<std::int64_t>(i % 9) - 4) * 97);
        CHECK(a.vy[i] == -(static_cast<std::int64_t>(i % 11) * 51 + 75));
    }
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
    // These arrays were populated by the genuine Oreslang guest. The
    // preallocated native presentation layout must preserve every bit,
    // including the epoch token, without implementing physics in C++.
    ores::Snapshot soa_frame{}, roundtrip{};
    ores::AoSoaSnapshot aosoa_frame{};
    ores::copy_snapshot(a, soa_frame, 7);
    soa_frame.generation = 19;
    ores::pack_snapshot_aosoa(soa_frame, aosoa_frame);
    ores::unpack_snapshot_aosoa(aosoa_frame, roundtrip);
    CHECK(roundtrip.tick == soa_frame.tick);
    CHECK(roundtrip.generation == soa_frame.generation);
    CHECK(roundtrip.missed_deadlines == soa_frame.missed_deadlines);
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        CHECK(roundtrip.x[i] == soa_frame.x[i]);
        CHECK(roundtrip.y[i] == soa_frame.y[i]);
        CHECK(roundtrip.radius[i] == soa_frame.radius[i]);
    }
    CHECK(digest(a) != 0);
    std::printf("PHYSICS_GUEST_1200_TICK_DIGEST=%016llx\n",
                static_cast<unsigned long long>(first_run_digest));
    std::puts("REAL_ORESLANG_LLVM_PHYSICS_CONTRACT_PASS");
}
