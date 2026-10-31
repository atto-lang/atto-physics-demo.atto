#include "PhysicsCore.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace {
// Host-only finite-domain certificate for INITIALIZATION PARAMETERS, not an
// executable substitute for any Oreslang physics function.
consteval bool initial_state_in_domain() {
    using namespace ores;
    for (std::size_t i = 0; i < kBodies; ++i) {
        const auto col = static_cast<std::int64_t>(i % 16);
        const auto row = static_cast<std::int64_t>(i / 16);
        const auto radius = (7 + static_cast<std::int64_t>(i % 7)) * kScale;
        const auto x = (36 + col * 57) * kScale;
        const auto y = (28 + row * 36) * kScale;
        const auto vx = (static_cast<std::int64_t>(i % 9) - 4) * 97;
        const auto vy = -(static_cast<std::int64_t>(i % 11) * 51 + 75);
        if (radius < kMinRadius || radius > kMaxRadius) return false;
        if (x < radius || x > kWidth - radius) return false;
        if (y < radius || y > kHeight - radius) return false;
        if (vx < -kMaxHorizontalSpeed || vx > kMaxHorizontalSpeed) return false;
        if (vy < -kMaxVerticalSpeed || vy > kMaxVerticalSpeed) return false;
    }
    return true;
}
static_assert(initial_state_in_domain(), "initial SoA arrays violate the bounded numeric envelope");
static_assert(ores::kMaxVerticalSpeed + ores::kGravityPerTick <
              std::numeric_limits<std::int64_t>::max());
static_assert(ores::kMaxHorizontalSpeed < std::numeric_limits<std::int64_t>::max());
}

int main() {
    std::puts("PHYSICS_HOST_NUMERIC_ENVELOPE_NO_GUEST_PASS");
}
