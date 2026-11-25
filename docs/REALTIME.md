# Hard real-time / zero tracing GC acceptance criteria

## Current implementation boundary

- No tracing garbage collector exists in the Oreslang scalar kernel or C++ simulation.
- World and snapshots occupy fixed-capacity standard arrays. They are
  initialized before the first simulation update.
- Each fixed step performs exactly 192 independently bounded iterations.
- No mutex, condition variable, blocking channel, Qt call, dynamic task creation
  or unbounded work queue is in `step_world`.
- There is one simulation producer and one UI consumer. The producer drops
  snapshot publication if the queue is full, rather than waiting for the UI.
- Physics timing uses an ordinary OS scheduling thread. Timer jitter and
  arbitrary preemption remain possible; Qt remains outside the RT boundary.
- The initial kernel handles wall contacts only, not interactions between bodies.
- Arithmetic overflow and native ABI failures are not yet formally excluded
  over every reachable state; whole-program invariants/WCET remain pending.

## Closed numeric envelope: current fixed initializer only

The new host-only `physics_numeric_envelope_no_guest_contract` checks the 192
initializer coordinates and radii at compile time. Bounds currently are:
- 1/256 pixel units, radius in [1792, 3328], width 245760, height 143360.
- Horizontal speed magnitude <= 388.
- Initial vertical speed magnitude <= 585; acceleration is +42/tick, terminal
  upward/downward signed limit at +1792; wall reflection can negate the current
  speed, so throughout the reachable run |vy| <= 1792.
- All starting centers are at least their radius from each boundary.
- Each step's unclamped coordinate sum is within [0, extent] because the radius
  is >= both speed bounds. Reflection preserves the speed envelope and the
  position clamp restores [radius, extent-radius].
- Signed negation, gravity addition and coordinate addition therefore stay
  well inside 64-bit signed range for a world initialized by `reset_world`.

These compile-time claims are conditional on the concrete initializer and
step logic remaining aligned, and cannot establish that a missing/unverified
Oreslang compiler produces conformant guest code. The linked guest contract
now also checks speed/radius/position envelopes on every tick, boundary
oracles, and deterministic 1200-tick reset/replay. It has NOT yet executed
against a pinned compiler at this head.

**Not proven:** arbitrary caller-supplied inputs to exported `.ores` functions,
arbitrary mutated World states, unbounded tick-counter lifetime, native guest
behavior, complete program memory safety, WCET, or thread scheduling.

## Evidence gates (all required before calling it hard real-time)

1. Run `oreslang-llvmc` on the actual `.ores` sources at pinned SHA.
   Show actual generated LLVM IR and native object linking into C++ and Qt.
2. Run the real guest ABI conformance test at `-O0` and `-O2`, with
   independently expected boundary cases and repeated deterministic state hashes.
3. Use ASan/UBSan on native host and guest at compatible toolchain revisions.
   Treat sanitizer omission as missing coverage, not success.
4. Instrument allocations after initialization (including `malloc`, C++ new,
   compiler runtime helpers, unwinding, task frames); prove zero allocation on
   each steady-state tick, including resource-exhaustion paths.
5. Prove fixed worst-case work and arithmetic bounds including initial-state,
   speed and body-count constraints. Reject unbounded source task creation.
6. Benchmark distribution, maximum and worst-case latency under controlled
   CPU affinity, workload and interference; distinguish observed maxima from WCET.
7. Establish priority, page-fault, scheduler policy, deadline miss handling and
   resource budgeting on an explicitly configured RTOS or RT Linux target.
8. Validate GUI overload, delayed UI consumer, shutdown and pause/reset races.
   The UI must never hold a resource required by the simulation's deadline.
9. Do not infer hard RT on other CPUs/OSes from a fast desktop demonstration.

## Future task-based parallel physics

A single-threaded deterministic reference must remain authoritative. Add
bounded worker pools and partition the SoA by disjoint mutable ranges only
after native Oreslang task capture/borrow checking is verified. Use barriers
between broadphase, narrowphase and contact-island solve phases. Test
serial-vs-parallel state hashes with a *defined* deterministic operation order,
rather than assuming floating-point atomics are deterministic. Keep all
cross-partition handoffs bounded and preallocated.

## Scope separation

The native simulation **aims** at hard RT; QML/Qt rendering is best effort.
Presentation may be delayed or dropped without affecting world ownership.
No statement that the complete desktop application is hard-RT certified
should be made based solely on the presence of a simulation worker thread.
