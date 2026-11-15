# atto-physics-demo.atto

Physics simulation examples exploring numerical computation and concurrency in AttoLang.

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

AttoLang is the public name; preserve existing Oreslang/.ores compatibility and use canonical .atto for new portable library source. This README defines scope, not feature readiness.

**Acceptance:** name exact source SHA/tree, runtime/toolchain, positive and negative checks and execution receipt before claiming support. Host-only tests and empty CI steps do not prove guest execution.
