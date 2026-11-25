> **AI agent instructions (organization-wide):** Before working on code, issues, or pull requests, read and follow the [organization `AGENTS.md`](https://github.com/atto-lang/.github/blob/main/AGENTS.md), including its **merge-first stealth-development policy**. Honor any repository-local `AGENTS.md`, explicit holds, and GitHub-enforced rules.

# atto-physics-demo.atto

Physics simulation examples exploring numerical computation and concurrency in AttoLang.

## Naming migration and language authority

This repository is maintained at [atto-lang/atto-physics-demo.atto](https://github.com/atto-lang/atto-physics-demo.atto); use `https://github.com/atto-lang/atto-physics-demo.atto.git` for Git remotes and canonical `.atto` source examples. The normative language contract is [atto-spec](https://github.com/atto-lang/atto-spec), with executable guest evidence tracked by [atto-platform-conformance](https://github.com/atto-lang/atto-platform-conformance). Follow current ownership, borrowing, effects, actor, and syntax rules there; do not infer implemented features from a README or unexecuted CI.

## Demonstration focus

Numerical simulation, state updates, and actor-safe computation.

## Running the example

Inspect the checked-in source, project manifest, and workflows for supported compiler backends and actual run commands. Source files use the intended `.atto` extension for AttoLang; backend support must be confirmed rather than presumed.

## Expected verification

Examples should document their dependencies, expected output, negative/error cases, and exact compiler/runtime versions used to verify execution. Host-side tests do not establish AttoLang guest execution.

## Status

This README describes the demonstration's intended purpose. Current executable coverage and feature support should be verified against code and CI.

## AttoLang ecosystem

- [AttoLang](https://github.com/atto-lang)
- [Specification](https://github.com/atto-lang/atto-spec)
- [Platform conformance](https://github.com/atto-lang/atto-platform-conformance)

## Repository ownership and contribution routing

**Belongs here:** A self-contained physics simulation in .atto, numerical integration examples, actor partitioning experiments and reproducible visuals/outputs.

**Belongs elsewhere:** General numerical primitives belong in atto-tensor/atto-data-science; benchmark contracts in domain-specific benchmark repos; runtime scheduling in compiler repos.

**Security and portability boundary:** State numerical tolerance, timestep, units, stability, bounded memory and exact backend evidence; avoid asserting deterministic cross-CPU results without tests.

AttoLang is the canonical project name; new source, examples and documentation use `.atto` and the `atto-lang` repository organization. This README defines scope, not feature readiness.

**Acceptance:** name exact source SHA/tree, runtime/toolchain, positive and negative checks and execution receipt before claiming support. Host-only tests and empty CI steps do not prove guest execution.

## Repository ownership and migration history

**AttoLang** is the current public language. This repository, `atto-lang/atto-physics-demo.atto`, contains migrated work from the archived `ores-truffle-oreslang/oreslang-physics-demo.ores` repository. Preserve working `.ores` files, repo naming and package/CLI compatibility until an independently tested change migrates them.

- **Owns:** AttoLang physics simulations, numerical examples and compiler-pinned test cases.
- **Not this repository:** proving backend numerical completeness by demos alone.
- **Code ownership:** [atto-lang](https://github.com/atto-lang) is the destination for language semantics, core/std, compilers, developer tools, libraries and demos. [atto-co](https://github.com/atto-co) independently owns managed infrastructure, tenancy, identity, billing and production deployments. A separate `atto-*` repo is not by itself a verified Git transfer or equivalent source tree.
- **Migration boundary:** the current AttoLang repository is the destination for this demo. Do not infer an unchanged numeric GitHub ID or a verified ownership transfer merely from the repository name or imported commits. A later production product spinout requires its own approval.

**Historical repository transfer playbook (for any future transfer):** coordinate in [issue #26](https://github.com/ores-truffle-oreslang/.github/issues/26) and review [private migration draft #25](https://github.com/ores-truffle-oreslang/.github/pull/25). Capture stable numeric ID, exact head SHA/tree, PRs, releases, packages, GitHub Pages, dependencies, branch/ruleset protections, Actions runners, secrets, App/webhook grants and rollback plan. Require explicit source and target administrators plus independent security reviewer. Never export/mirror private sources as a shortcut. After an actual GitHub ownership transfer, verify unchanged ID/history/tree, re-bind security controls and run exact-head CI. A job with `steps=[]` or `steps=null` is `NOT_EXECUTED`, and host/reference results do not prove compiled AttoLang guest or production HTTP execution.

## Native LLVM + Qt Quick implementation candidate

The imported legacy implementation uses a **native Oreslang** physics kernel, intended to be compiled by
`oreslang-llvmc` to LLVM IR and then to a machine-code object with Clang. Qt Quick
renders the bodies and supplies the QML skin. There is no Java runtime, no embedded
webview, no tracing garbage collector, and no actor dependency in the simulation.

**Status (2026-10-30): implementation candidate, not yet executable-verified.**
The repository uses only the currently admitted native *scalar* ABI. It does
**not** claim that source-level inverted SoA collections, parallel Oreslang Tasks,
collision islands, rendering APIs, or hard-real-time WCET proofs are implemented.

### Separation of responsibilities

| Layer | Implementation | Steady-state behavior |
| --- | --- | --- |
| Motion, gravity and boundary responses | `src/physics/physics.ores` | Native AOT i64 calculations |
| Fixed-capacity SoA and frame dispatch | `src/native/PhysicsCore.cpp` | No heap allocation during ticks |
| Scheduling and snapshot transport | `PhysicsEngine.cpp` | Dedicated thread, fixed step, bounded SPSC queue |
| Rendering/UI | Qt 6 + `ui/Main.qml` | May allocate; never runs in the simulation thread |

The C++ adapter owns **storage and scheduling**, not the physics formulas. All
live physics calls are linked against actual compiler-generated Oreslang symbols.
This simulation candidate targets **192 independent bouncing circles with gravity and wall
collisions**; integrated body-to-body collision response is **not implemented**. The simulation is
fixed-point (1/256 pixel) to avoid floating-point portability variability.

### Prerequisites

- The actual `oreslang-llvmc` binary built from
  [oreslang-source.llvm](https://github.com/ores-truffle-oreslang/oreslang-source.llvm)
  available on `PATH` or explicitly passed to CMake
- Clang that can consume LLVM IR from that compiler
- CMake 3.20+, a C++20 compiler
- Qt 6.2+ Quick, Qml, QuickControls2 development packages for the desktop
- Linux/macOS/Windows toolchain supported by your installed Qt and LLVM setup

#### Build native execution proof without Qt

```sh
cmake -S . -B build/headless -DORES_PHYSICS_GUI=OFF \
  -DORESLANG_LLVM_COMPILER=/absolute/path/to/oreslang-llvmc
cmake --build build/headless --parallel 2
ctest --test-dir build/headless --output-on-failure
```

The build **must fail** if real Oreslang compilation, the required guest ABI
symbols, Clang IR compilation, native linking, or runtime assertions fail.
It never falls back to stand-in physics implemented in another language.

#### Build QML desktop app

```sh
cmake -S . -B build/desktop \
  -DORESLANG_LLVM_COMPILER=/absolute/path/to/oreslang-llvmc
cmake --build build/desktop --parallel 2
./build/desktop/oreslang_physics
```

On some platforms the executable is inside a configuration subdirectory
(for example `build/desktop/Release/`).

### Real-time scope

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

### Build discipline

No `python` support scripts are introduced. No fake compiler, host-language
physics fallback or skipped native integration tests are accepted as a pass.
Keep provenance of source revision, LLVM compiler revision, Clang version,
Qt version, platform, test output, and sanitizer runs for any verification
claim. A screenshot of animated circles proves rendering, not physics safety.
