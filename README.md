# Oreslang Physics Demo — native LLVM + Qt Quick

A desktop physics playground with a **native Oreslang** physics kernel, compiled by
`oreslang-llvmc` to LLVM IR and then to a machine-code object with Clang. Qt Quick
renders the bodies and supplies the QML skin. There is no Java runtime, no embedded
webview, no tracing garbage collector, and no actor dependency in the simulation.

**Status (2026-10-30): implementation candidate, not yet executable-verified.**
The repository uses only the currently admitted native *scalar* ABI. It does
**not** claim that source-level inverted SoA collections, parallel Oreslang Tasks,
collision islands, rendering APIs, or hard-real-time WCET proofs are implemented.

## Separation of responsibilities

| Layer | Implementation | Steady-state behavior |
| --- | --- | --- |
| Motion, gravity and boundary responses | `src/physics/physics.ores` | Native AOT i64 calculations |
| Fixed-capacity SoA and frame dispatch | `src/native/PhysicsCore.cpp` | No heap allocation during ticks |
| Scheduling and snapshot transport | `PhysicsEngine.cpp` | Dedicated thread, fixed step, bounded SPSC queue |
| Rendering/UI | Qt 6 + `ui/Main.qml` | May allocate; never runs in the simulation thread |

The C++ adapter owns **storage and scheduling**, not the physics formulas. All
live physics calls are linked against actual compiler-generated Oreslang symbols.
This first slice has **192 independent bouncing circles with gravity and wall
collisions**; body-to-body collisions are **not implemented**. The simulation is
fixed-point (1/256 pixel) to avoid floating-point portability variability.

## Prerequisites

- The actual `oreslang-llvmc` binary built from
  [oreslang-source.llvm](https://github.com/ores-truffle-oreslang/oreslang-source.llvm)
  available on `PATH` or explicitly passed to CMake
- Clang that can consume LLVM IR from that compiler
- CMake 3.20+, a C++20 compiler
- Qt 6.2+ Quick, Qml, QuickControls2 development packages for the desktop
- Linux/macOS/Windows toolchain supported by your installed Qt and LLVM setup

### Build native execution proof without Qt

```sh
cmake -S . -B build/headless -DORES_PHYSICS_GUI=OFF \
  -DORESLANG_LLVM_COMPILER=/absolute/path/to/oreslang-llvmc
cmake --build build/headless --parallel 2
ctest --test-dir build/headless --output-on-failure
```

The build **must fail** if real Oreslang compilation, the three guest ABI
symbols, Clang IR compilation, native linking, or runtime assertions fail.
It never falls back to stand-in physics implemented in another language.

### Build QML desktop app

```sh
cmake -S . -B build/desktop \
  -DORESLANG_LLVM_COMPILER=/absolute/path/to/oreslang-llvmc
cmake --build build/desktop --parallel 2
./build/desktop/oreslang_physics
```

On some platforms the executable is inside a configuration subdirectory
(for example `build/desktop/Release/`).

## Real-time scope

**Hard-real-time capable design, not a hard-real-time guarantee.**
Fixed-size memory and bounded per-tick loop cardinality are necessary but
insufficient. The current thread uses a standard scheduler and
`sleep_until`, and Qt UI/render timing is explicitly nondeterministic.
No WCET, lock-free progress bound under all OS conditions, latency ceiling,
schedulability analysis, or binary-level no-allocation proof has passed.

The SPSC queue drops visual snapshots rather than stalling physics. Missed
simulation deadlines and dropped rendering snapshots are separately counted.
A fixed-point update is never retried merely because rendering is late.

See [real-time acceptance gates](docs/REALTIME.md) for the criteria required
before any hard-RT claim; [LLVM gaps](docs/LLVM_GAPS.md) tracks the path to
Oreslang-owned inverted SoA, structured tasks and native multi-body physics.

## Build discipline

No `python` support scripts are introduced. No fake compiler, host-language
physics fallback or skipped native integration tests are accepted as a pass.
Keep provenance of source revision, LLVM compiler revision, Clang version,
Qt version, platform, test output, and sanitizer runs for any verification
claim. A screenshot of animated circles proves rendering, not physics safety.
