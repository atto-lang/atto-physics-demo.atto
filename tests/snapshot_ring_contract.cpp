#include "SnapshotRing.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#define CHECK(cond) do { if (!(cond)) { \
    std::fprintf(stderr, "snapshot ring check failed at line %d: %s\n", __LINE__, #cond); \
    std::abort(); \
} } while (false)

static void fill(ores::Snapshot& snapshot, std::uint64_t tick) {
    snapshot.tick = tick;
    snapshot.missed_deadlines = tick ^ 0x55aa55aaULL;
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        snapshot.x[i] = static_cast<std::int64_t>(tick + i);
        snapshot.y[i] = -static_cast<std::int64_t>(tick + i);
        snapshot.radius[i] = static_cast<std::int64_t>(i + 1);
    }
}

static void validate(const ores::Snapshot& snapshot) {
    CHECK(snapshot.missed_deadlines == (snapshot.tick ^ 0x55aa55aaULL));
    for (std::size_t i = 0; i < ores::kBodies; ++i) {
        CHECK(snapshot.x[i] == static_cast<std::int64_t>(snapshot.tick + i));
        CHECK(snapshot.y[i] == -static_cast<std::int64_t>(snapshot.tick + i));
        CHECK(snapshot.radius[i] == static_cast<std::int64_t>(i + 1));
    }
}

int main() {
    {
        // Controller-owned stop/restart: old generation frames and telemetry
        // cannot contaminate the first snapshot of a new worker.
        ores::SnapshotRing queue;
        ores::Snapshot frame{}, read{};
        for (std::uint64_t generation = 1; generation <= 1024; ++generation) {
            fill(frame, generation * 1000 + 100);
            CHECK(queue.publish(frame));
            fill(frame, generation * 1000 + 101);
            CHECK(queue.publish(frame));
            fill(frame, generation * 1000 + 102);
            CHECK(queue.publish(frame));
            fill(frame, generation * 1000 + 103);
            CHECK(!queue.publish(frame));
            CHECK(queue.skipped_snapshots() == 1);
            // No producer/consumer is active while the controller resets.
            queue.clear_quiescent();
            CHECK(queue.skipped_snapshots() == 0);
            CHECK(!queue.consume_latest(read));
            fill(frame, generation);
            CHECK(queue.publish(frame));
            CHECK(queue.consume_latest(read));
            CHECK(read.tick == generation);
            validate(read);
            CHECK(!queue.consume_latest(read));
        }
    }
    {
        ores::SnapshotRing queue;
        ores::Snapshot sample{}, result{};
        CHECK(!queue.consume_latest(result));
        for (std::uint64_t tick = 1; tick <= 3; ++tick) {
            fill(sample, tick);
            CHECK(queue.publish(sample));
        }
        fill(sample, 4);
        CHECK(!queue.publish(sample));  // Full queue does not block.
        CHECK(queue.skipped_snapshots() == 1);
        CHECK(queue.consume_latest(result));
        CHECK(result.tick == 3);
        validate(result);
        CHECK(queue.skipped_snapshots() == 3);  // One dropped + two superseded.
        CHECK(!queue.consume_latest(result));
        CHECK(queue.publish(sample));
        CHECK(queue.consume_latest(result));
        CHECK(result.tick == 4);
        for (std::uint64_t i = 5; i < 100'005; ++i) {
            fill(sample, i);
            CHECK(queue.publish(sample));
            CHECK(queue.consume_latest(result));
            CHECK(result.tick == i);
        }
    }
    {
        ores::SnapshotRing queue;
        std::atomic<bool> finished{false};
        std::atomic<std::uint64_t> received{0};
        std::thread producer([&] {
            ores::Snapshot sample{};
            for (std::uint64_t i = 1; i <= 30'000; ++i) {
                fill(sample, i);
                (void)queue.publish(sample);
            }
            finished.store(true, std::memory_order_release);
        });
        std::thread consumer([&] {
            ores::Snapshot sample{};
            std::uint64_t previous = 0;
            while (true) {
                if (queue.consume_latest(sample)) {
                    validate(sample);
                    CHECK(sample.tick > previous);
                    previous = sample.tick;
                    received.fetch_add(1, std::memory_order_relaxed);
                } else if (finished.load(std::memory_order_acquire)) {
                    break;
                } else {
                    std::this_thread::yield();
                }
            }
        });
        producer.join();
        consumer.join();
        CHECK(received.load(std::memory_order_relaxed) > 0);
    }
    std::puts("SNAPSHOT_RING_NO_GUEST_CONTRACT_PASS");
}
