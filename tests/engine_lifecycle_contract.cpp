#include "PhysicsEngine.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#define CHECK(c) do { if (!(c)) { \
    std::fprintf(stderr, "engine lifecycle check failed at %s:%d: %s\n", \
                 __FILE__, __LINE__, #c); std::abort(); \
} } while (false)

namespace {
using clock = std::chrono::steady_clock;

void check_frame(const ores::Snapshot& frame, std::uint64_t generation) {
    CHECK(frame.generation == generation);
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        CHECK(frame.radius[i] >= ores::kMinRadius);
        CHECK(frame.radius[i] <= ores::kMaxRadius);
        CHECK(frame.x[i] >= frame.radius[i]);
        CHECK(frame.x[i] <= ores::kWidth - frame.radius[i]);
        CHECK(frame.y[i] >= frame.radius[i]);
        CHECK(frame.y[i] <= ores::kHeight - frame.radius[i]);
    }
}

void await_tick(ores::PhysicsEngine& engine, ores::Snapshot& frame,
                std::uint64_t generation, std::uint64_t min_tick) {
    const auto until = clock::now() + std::chrono::seconds(3);
    while (clock::now() < until) {
        if (engine.read_latest(frame)) {
            check_frame(frame, generation);
            if (frame.tick >= min_tick) return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::fprintf(stderr, "timed out awaiting guest frame generation %llu tick %llu\n",
                 static_cast<unsigned long long>(generation),
                 static_cast<unsigned long long>(min_tick));
    std::abort();
}
}  // namespace

int main() {
    // Links actual oreslang-llvmc-generated guest object. No synthetic .ores
    // implementation and no host-language physics substitute.
    ores::PhysicsEngine engine;
    ores::Snapshot frame{};
    for (std::uint64_t generation = 1; generation <= 12; ++generation) {
        engine.set_paused(false);
        engine.start();
        engine.start();  // Idempotent, one controller thread.
        await_tick(engine, frame, generation, 4);
        engine.set_paused(true);
        engine.request_reset();
        // The worker handles reset while paused, and publishes tick=0.
        const auto until = clock::now() + std::chrono::seconds(3);
        bool saw_reset = false;
        while (clock::now() < until) {
            if (engine.read_latest(frame)) {
                check_frame(frame, generation);
                if (frame.tick == 0) { saw_reset = true; break; }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        CHECK(saw_reset);
        engine.set_paused(false);
        await_tick(engine, frame, generation, 2);
        engine.stop();
        engine.stop();  // Idempotent; joins before the next start.
        // New worker must clear leftover frames before publishing next epoch.
        // A stale frame from this epoch may remain now, intentionally.
    }
    std::puts("REAL_ORESLANG_LLVM_PHYSICS_LIFECYCLE_CONTRACT_PASS");
}
