#include "SnapshotLayouts.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

#define CHECK(c) do { if (!(c)) { \
    std::fprintf(stderr, "snapshot AoSoA check failed at line %d: %s\n", \
                 __LINE__, #c); std::abort(); \
} } while (false)

// Host-only proof that the bounded pack/unpack functions do not invoke the
// normal C++ heap allocator. This is NOT guest-compiler/RT allocation evidence.
static std::uint64_t allocations = 0;
void* operator new(std::size_t size) {
    ++allocations;
    if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t size) {
    ++allocations;
    if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

static void verify(const ores::Snapshot& lhs, const ores::Snapshot& rhs) {
    CHECK(lhs.tick == rhs.tick);
    CHECK(lhs.generation == rhs.generation);
    CHECK(lhs.missed_deadlines == rhs.missed_deadlines);
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        CHECK(lhs.x[i] == rhs.x[i]);
        CHECK(lhs.y[i] == rhs.y[i]);
        CHECK(lhs.radius[i] == rhs.radius[i]);
    }
}

int main() {
    ores::Snapshot source{}, recovered{};
    ores::AoSoaSnapshot packed{};
    const auto before = allocations;
    for (std::uint64_t tick = 0; tick < 4096; ++tick) {
        source.tick = tick;
        source.generation = 1 + (tick / 123);
        source.missed_deadlines = tick / 73;
        for (std::size_t i = 0; i < ores::kBodies; ++i) {
            // Inject both negative and positive bounded signed coordinates,
            // including all 16 AoSoA lanes and every block boundary.
            const auto offset = static_cast<std::int64_t>((tick * 17 + i * 193) % 4096);
            source.x[i] = offset - 2048;
            source.y[i] = 1024 - offset;
            source.radius[i] = static_cast<std::int64_t>(i % 7) + 1;
        }
        ores::pack_snapshot_aosoa(source, packed);
        ores::unpack_snapshot_aosoa(packed, recovered);
        verify(source, recovered);
    }
    CHECK(before == allocations);
    std::puts("PHYSICS_SNAPSHOT_AOSOA_LAYOUT_NO_GUEST_PASS");
}
