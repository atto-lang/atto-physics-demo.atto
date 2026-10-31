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
`scripts/run_pinned_llvm_guest_matrix.sh` with all six mandatory environment
variables: `EXPECTED_PHYSICS_COMMIT_SHA`, `EXPECTED_PHYSICS_TREE_SHA`,
`EXPECTED_LLVM_COMPILER_SHA256`, `EXPECTED_CLANG_SHA256`,
`ORESLANG_LLVM_COMPILER` and `ORE_CLANG`.
The SHA values must be independently approved before execution. The script
rejects a dirty or mismatched physics checkout, invalid executable identities,
missing pins and missing binaries. It configures independent guest O0 and O2
builds, invokes CTest against real compiler-generated native objects, and
compares the printed 1200-tick world digest. Both paths must emit the real-guest
PASS marker; a host-only test cannot produce a valid result.

The runner prints source commit/tree and input/output SHA-256 values for a
receipt. It does not acquire compilers, copy private source, approve another
organization, establish OS/hardware WCET, or certify zero allocations.

## Fresh guest build provenance (stacked hardening gate)

The real O0/O2 runner now requires both `EXPECTED_PHYSICS_COMMIT_SHA` and
`EXPECTED_PHYSICS_TREE_SHA` (40 lowercase hexadecimal digits each), plus
independently approved SHA-256 pins for `oreslang-llvmc` and Clang. Retrieve
the exact tree with `git rev-parse 'HEAD^{tree}'` in the authorized source
repository; do not infer approval from a matching commit alone.

After validating the source commit/tree, clean worktree and both compiler
digests, the runner allocates a **fresh unique private build directory** under
`build/pinned-native-guest.XXXXXXXX`, with O0 and O2 subdirectories. It
rejects a symlinked `build/` root and preserves any previous receipts rather
than deleting them. Reusing an old IR/object/native-contract executable cannot
satisfy this runner's newly allocated build directory. Clean up older build
directories only under a separate reviewed retention policy.

An optimization-level change in a separately managed CMake build directory
also changes `native-physics-opt-level.txt`; the native object depends on
that file. A host-only CMake configure contract checks O0→O2 in the **same**
build directory without running a compiler or fabricating physics IR.

The source-free negative runner contract tests wrong HEAD, wrong tree, dirty
worktree, compiler digest mismatch, Clang digest mismatch and a symlinked
build root. Synthetic sentinel programs are not used as physics compiler
substitutes; no native guest proof follows from those checks.

**Current evidence boundary:** the runner's freshness and pin guards are
host/source contracts only. Nothing here proves a real compiler produced
correct IR, that Qt ran, that sanitizers passed, or that a hard RT deadline
is satisfied. Keep the stacked PRs draft until independent exact-head native
execution and review have been recorded.

## Worker restart and snapshot-generation safety

Every worker launch now gets a monotonic 64-bit generation. The worker
captures it by value and stamps every published snapshot, separate from the
resettable physics tick. Old SPSC frames and skipped counters are discarded
**only at a controller-owned quiescent point** immediately before launching
the next producer. Never call `SnapshotRing::clear_quiescent()` with a
concurrent producer or reader; `start()`/`stop()`/`read_latest()` retain their
one-GUI-controller-thread ownership contract. Generation exhaustion throws
before starting a worker rather than wrapping.

The source-only queue contract adds 1024 reset cycles including full ring,
skipped count, stale frame rejection and post-reset publication.

The **real guest** lifecycle test now requires 12 worker generations with
actual Oreslang-linked physics code; it verifies advancing ticks, paused reset
to tick zero, frame spatial bounds and no prior-generation frame escapes. It
is only built with `ORES_PHYSICS_QUEUE_ONLY=OFF`. It is not a passing test
until the real native guest is compiled and run.

## Preallocated snapshot AoSoA transport

A separate 16-lane, 64-byte-aligned AoSoA layout converts presentation
snapshots to/from the existing SoA representation with no heap allocation
inside either conversion. A host-only 4096-frame contract checks every
coordinate, radius, tick, generation and deadline counter through a
round-trip, plus C++ global new/delete interposition for those loops.
The real guest contract additionally checks one snapshot produced from
the actual physics world. This is **not** an alternate AoSoA physics solver
and does **not** establish serial/parallel engine parity, general malloc
interposition or WCET.

See `docs/TEN_GATE_LEDGER.md` for ten numbered promotion gates and exact
missing evidence for each. Keep native compiler, Qt, TSAN and target
real-time claims explicitly marked unverified until executed.


## Static LLVM IR body-admission regression (source-only)

The ABI preflight now rejects a bare `define i64 @function(...)` header if
it has no opening function body, including braces in an LLVM IR comment. It
permits attributes between the signature and the body opener and an optional
line break before the brace. Two negative mutation fixtures lock down those
cases; the original arity/type/linkage and comment-vs-declaration tests remain.

This is only a structural IR gate. A fully valid LLVM IR module still requires
real compiler output and a native Clang parse/build followed by execution.
Source-equivalent local CMake fixtures and Clang-generated C-reference IR
cannot substitute for `.ores` guest compilation or exact-head GitHub Actions.

At Clang O2 a valid function may acquire an ABI-neutral `range(i64 0, 2)`
return attribute (for an integer-valued predicate). The preflight therefore
accepts this narrowly specified attribute before `i64`, while refusing
incorrect range widths and ABI-modifying `zeroext` returns. The exact source
blobs were locally tested against C-reference Clang `-O0` and `-O2` IR;
this proves structural compatibility only, not Oreslang guest execution.
