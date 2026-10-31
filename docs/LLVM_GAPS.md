# LLVM compiler capability and follow-on milestones

This demo intentionally uses admitted native scalar functions with external
C ABI symbols. It does **not** invent a stable C ABI for arbitrary `.ores`
arrays, classes, actors, tasks or `async/await`.

## Milestones

1. **P0 — Native proof:** Compile the three `.ores` functions with
   `oreslang-llvmc` and Clang, run `physics_native_contract`, capture exact
   source/compiler/toolchain hashes and test results. Compile and open the Qt
   frontend. Current status: staged, not executed.
2. **P1 — Native owned SoA:** Move the columns `x/y/vx/vy/radius` from the
   bounded C++ ABI adapter into Oreslang-owned inverted collections, with
   compiler-proved lifetimes. No mandatory mutation copies.
3. **P2 — Collisions:** Broadphase grid, narrowphase circle contact generation,
   bounded contact island solver, independent oracle and invariant tests.
   Maintain the scalar golden reference.
4. **P3 — Parallelism:** Verified Oreslang Tasks with disjoint range borrows,
   bounded task pool and structured joins. No actor requirement.
5. **P4 — Real-time audit:** allocation instrumented AOT builds, sanitizer
   evidence, maximum-work analysis and hardware/OS-specific scheduling proof.
6. **P5 — Portability:** Share deterministic fixtures with .java/.wasm/.net/
   .forge and clearly separate compiler-compatible from executable proof.

## Why not start with source-level tasks or native QML calls?

The existing LLVM README describes incomplete general async/task and
source array features. QML uses Qt-specific object and render-thread
interfaces, so a thin native C++ Qt binding is a legitimate platform
boundary. Claiming direct Oreslang QML interop without a tested compiler
lowering would make the demo misleading.
