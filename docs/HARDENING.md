# Physics demo hardening and evidence gates

This branch hardens the native C++ host boundary of the **actual** Oreslang LLVM
physics demonstration. It is not a replacement physics implementation.

## Implemented

- `SnapshotRing`: fixed-capacity SPSC latest-frame transport. Producer never
  blocks if Qt falls behind; ownership and acquire/release publication prevent
  producer writes from racing consumer copies. Indices and telemetry atomics
  must be lock-free on the selected machine.
- Qt refresh/paint use a private UI-only mutex to publish a complete, immutable
  rendering snapshot. The simulation thread never acquires that mutex.
- The canvas joins the physics thread explicitly on destruction.
- Skipped-frame telemetry counts both producer-overflow drops and consumer-
  superseded frames.
- A standalone, compiler-independent ring contract covers full queues, skipped
  counts, payload coherence, repeated ring reuse and threaded producer/consumer.
- The regular guest build still requires the **real** `oreslang-llvmc`; the
  queue-only CMake mode is explicitly named and cannot be used as guest proof.

## Standalone bounded transport gate

```sh
cmake -S . -B build/queue -DORES_PHYSICS_GUI=OFF \
  -DORES_PHYSICS_QUEUE_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/queue --parallel 2
ctest --test-dir build/queue --output-on-failure
```

This gate is meaningful for C++ snapshot transport, but is **not** a test of
the Oreslang compiler, native guest arithmetic, Qt painting, or hard RT.

## Host-only IR preflight (not guest execution)

`ctest` in queue-only mode also runs a fixture-based LLVM IR ABI parser contract.
It rejects comment-only/declaration-only symbols, incorrect argument widths or
arity, internal linkage and an incompatible calling convention. The build
checks compiler-produced IR using the same verifier; passing fixture tests
is **not** evidence that Oreslang compiled, linked, or executed.

## Conditional fixed-point domain gate (no guest)

The queue-only CTest set now also includes a C++20 consteval check covering all
192 initializer slots, fixed-point scale, radius, position and velocity bounds.
The core header enforces compile-time constants that close under gravity,
wall reflection and clamping. These are source-level invariants for the
reset-world state machine and are **not** a proof about arbitrary public FFI
inputs, unbounded tick counters, hard RT or actual compiled Oreslang behavior.

The compiler-backed `tests/native_contract.cpp` remains mandatory and now
checks every body's domain on each of 1200 ticks, extra wall/terminal
boundary cases and replay after reset. Without exact-head real `.ores` guest
execution that test is an unexecuted conformance gate, not a pass.

## Required guest execution gate

```sh
cmake -S . -B build/guest -DORES_PHYSICS_GUI=OFF \
  -DORES_PHYSICS_QUEUE_ONLY=OFF \
  -DORESLANG_LLVM_COMPILER=/absolute/path/to/oreslang-llvmc
cmake --build build/guest --parallel 2
ctest --test-dir build/guest --output-on-failure
```

Capture the demo commit, compiler commit, emitted IR, compiler and Clang
versions and complete logs. Build at O0/O2, run deterministic and boundary
tests, then build the real Qt frontend separately. No stand-in C++/.ll guest
physics is allowed.

## Open blockers before real-time claims

1. Real `.ores` compilation, LLVM IR and native binary execution on pinned
   compiler and demo revisions.
2. Qt 6 GUI build, QML load and stop/reset/paint stress under actual rendering,
   plus a supported ThreadSanitizer pass. ASan/UBSan alone do not catch races.
3. Binary-level per-tick allocation measurement including guest-generated code
   and runtime helpers. Fixed-size C++ arrays alone are not a proof of zero
   allocation; zero tracing GC is not the same claim as zero allocation.
4. Prove reachable input domains and signed arithmetic safety for position,
   velocity, radius and gravity over the entire lifetime of the world.
5. Bound WCET, scheduler delays, page faults, thread priority and interference
   on an explicitly selected real-time OS and hardware platform. The desktop
   scheduler's `sleep_until` does not give a hard-deadline guarantee.
6. Prove render-thread teardown and ownership/lifetime behavior under rapid
   window close, GUI starvation and repeated pause/reset.
7. Implement and validate body-to-body collisions, broadphase and task
   parallelism separately; they are absent from the current scalar demo.

## Runner integrity

A CI job that fails before the first step cannot prove implementation failure
or success. If source-org Actions cannot admit jobs, use only an authorized
**private** carrier and verify every tree/blob at the exact tested SHA. Never
expose private source in public mirrors or persist credentials in artifacts.

## Verified IR publication and incremental build integrity

The compiler writes to a `.candidate` path, not the eventual `native-physics.ll`.
A fresh, independent CMake process verifies the candidate's LLVM signature
before the script atomically renames it to the final output. A nonzero compiler
exit or ABI rejection removes both the candidate and any stale final IR.
This ensures a rejected compiler invocation cannot leave an apparently valid
build output for a subsequent incremental build. The build graph also depends
on the compiler executable and ABI verifier script; changing either forces
recompilation rather than reusing stale IR. The object target depends on Clang.

`llvm_ir_compilation_fail_closed_no_guest_contract` exercises negative-only
compiler failure and, on Unix, the rejection of zero-exit non-IR output.
It intentionally does not generate valid guest code or establish LLVM execution.
For successful compilation and runtime proof, use the separate pinned real-guest
gate below. This does not prevent out-of-band artifact tampering; protect build
directories, provenance, and the compiler binary in the execution environment.

## Reproducible native O0/O2 execution matrix (not yet executed)

The native object optimization is explicit via `ORES_PHYSICS_GUEST_OPT_LEVEL`
(only 0 or 2). `CMAKE_BUILD_TYPE=Debug` alone does not set guest optimization.
`guest_optimization_config_no_guest_contract` tests positive O0/O2 and
negative/invalid modes without compiling or fabricating guest IR.

On an authorized Linux checkout with a genuine pinned compiler and Clang, run
`scripts/run_pinned_llvm_guest_matrix.sh` with all five mandatory environment
variables: `EXPECTED_PHYSICS_COMMIT_SHA`, `EXPECTED_LLVM_COMPILER_SHA256`,
`EXPECTED_CLANG_SHA256`, `ORESLANG_LLVM_COMPILER` and `ORE_CLANG`.
The SHA values must be independently approved before execution. The script
rejects a dirty or mismatched physics checkout, invalid executable identities,
missing pins and missing binaries. It configures independent guest O0 and O2
builds, invokes CTest against real compiler-generated native objects, and
compares the printed 1200-tick world digest. Both paths must emit the real-guest
PASS marker; a host-only test cannot produce a valid result.

The runner prints source commit/tree and input/output SHA-256 values for a
receipt. It does not acquire compilers, copy private source, approve another
organization, establish OS/hardware WCET, or certify zero allocations.

### Fresh-build provenance hardening

Each pinned-matrix invocation creates a fresh, uniquely named private build root
under ignored `build/`, prints its location in the receipt, and compiles separate
`O0` and `O2` guest artifacts there. Previous IR/object/executable files cannot
be reused by accident. Existing `build/` symlinks are rejected, and CTest must
contain at least one test. Generated build directories are retained for review;
clean them up only after archiving the independently approved evidence.
