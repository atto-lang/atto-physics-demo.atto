#include "PhysicsEngine.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

#define CHECK(c) do { if (!(c)) { \
  std::fprintf(stderr, "SPSC check failed: %s\n", #c); std::abort(); \
} } while (false)

int main() {
    ores::PhysicsEngine engine;
    engine.start();
    ores::Snapshot snap{};
    bool had_frame = false;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    std::uint64_t largest = 0;
    while (std::chrono::steady_clock::now() < until && largest < 6) {
        if (engine.read_latest(snap)) {
            had_frame = true;
            CHECK(snap.generation == 1);
            CHECK(snap.tick >= largest);
            largest = snap.tick;
            for (std::size_t i = 0; i < ores::kBodies; ++i) {
                CHECK(snap.x[i] >= snap.radius[i]);
                CHECK(snap.x[i] <= ores::kWidth - snap.radius[i]);
                CHECK(snap.y[i] >= snap.radius[i]);
                CHECK(snap.y[i] <= ores::kHeight - snap.radius[i]);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    engine.set_paused(true);
    engine.request_reset();
    engine.set_paused(false);
    engine.stop();
    CHECK(had_frame && largest >= 6);
    std::puts("ORESLANG_SPSC_NO_UI_BLOCKING_SMOKE_PASS");
}
