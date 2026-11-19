> **AI agent instructions (organization-wide):** Before working on code, issues, or pull requests, read and follow the [organization `AGENTS.md`](https://github.com/ores-truffle-oreslang/.github/blob/main/AGENTS.md), including its **merge-first stealth-development policy**. Honor any repository-local `AGENTS.md`, explicit holds, and GitHub-enforced rules.

# Oreslang Physics Demo (LLVM + Qt Quick)

## Oreslang to AttoLang migration (2026-11-18)

**AttoLang** (formerly **Oreslang**) is the current public language identity. This `ores-truffle-oreslang/oreslang-physics-demo.ores` checkout retains old naming. Its AttoLang counterpart for new contributions is [atto-lang/atto-physics-demo.atto](https://github.com/atto-lang/atto-physics-demo.atto); this link **does not imply an in-place transfer, identical history, migrated issues/PRs or feature parity**. The old source and commands below remain useful as historical checkout documentation, not a guarantee of current language conformance.

**[atto-lang/atto-spec](https://github.com/atto-lang/atto-spec)** is the canonical language specification. The portable library source belongs in [atto-common-source](https://github.com/atto-lang/atto-common-source), shared positive/negative guest tests in [atto-platform-conformance](https://github.com/atto-lang/atto-platform-conformance), and compiler-specific implementations in `atto-source.*`. The language's current proposed direction includes canonical `.atto`, `snake_case`, `borrow!`/`take!`/`share!`/`copy!`/`copy!!`, [implicit borrow defaults](https://github.com/atto-lang/atto-spec/blob/main/language/implicit-argument-borrow-v1.md), [`define const` including permitted `os.read_env`](https://github.com/atto-lang/atto-spec/blob/main/language/file-module-const-v1.md), and [`<<<`/`>>>`/`ashr` shifts vs. `<<`/`>>` streams](https://github.com/atto-lang/atto-spec/blob/main/language/bitwise-and-stream-operators-v1.md). Check individual spec admission status; **a design target is not proof of execution**.

Keep legacy `.ores` fixtures, CLI/package/ABI names, and version-pinned commands where they document code still in this tree. Avoid Python in new AttoLang tooling/tests/CI; Rust support tools are preferred. Only claim cross-backend support with exact SHA/tree, target/toolchain, real executed positive and negative guest tests; report failed, unsupported and not-executed explicitly. Empty-step/host-only CI is not guest conformance.

A native desktop physics playground built from Oreslang source compiled through LLVM, with a Qt Quick/QML interface.

**Status:** repository initialized; native simulation, host adapter, conformance tests and QML frontend are being staged on a feature branch. A successful visual demo or hard real-time guarantee has not yet been verified.

Design constraints: no actors required for physics; fixed-step simulation; bounded preallocated state; zero tracing GC in the simulation; thin native Qt rendering bridge. The GUI is explicitly **not** a hard-real-time component.

See the feature branch / pull request for the implementation and executable verification gates.
