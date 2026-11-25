# Oreslang Physics Demo (LLVM + Qt Quick)

A native desktop physics playground built from Oreslang source compiled through LLVM, with a Qt Quick/QML interface.

**Status:** repository initialized; native simulation, host adapter, conformance tests and QML frontend are being staged on a feature branch. A successful visual demo or hard real-time guarantee has not yet been verified.

Design constraints: no actors required for physics; fixed-step simulation; bounded preallocated state; zero tracing GC in the simulation; thin native Qt rendering bridge. The GUI is explicitly **not** a hard-real-time component.

See the feature branch / pull request for the implementation and executable verification gates.

## Repository ownership and migration (2026-11-08)

**AttoLang** is the current public language. This repository, `atto-lang/atto-physics-demo.atto`, contains migrated work from the archived `ores-truffle-oreslang/oreslang-physics-demo.ores` repository. Preserve working `.ores` files, repo naming and package/CLI compatibility until an independently tested change migrates them.

- **Owns:** AttoLang physics simulations, numerical examples and compiler-pinned test cases.
- **Not this repository:** proving backend numerical completeness by demos alone.
- **Code ownership:** [atto-lang](https://github.com/atto-lang) is the destination for language semantics, core/std, compilers, developer tools, libraries and demos. [atto-co](https://github.com/atto-co) independently owns managed infrastructure, tenancy, identity, billing and production deployments. A separate `atto-*` repo is not by itself a verified Git transfer or equivalent source tree.
- **Migration boundary:** the current AttoLang repository is the destination for this demo. Do not infer an unchanged numeric GitHub ID or a verified ownership transfer merely from the repository name or imported commits. A later production product spinout requires its own approval.

**Historical repository transfer playbook (for any future transfer):** coordinate in [issue #26](https://github.com/ores-truffle-oreslang/.github/issues/26) and review [private migration draft #25](https://github.com/ores-truffle-oreslang/.github/pull/25). Capture stable numeric ID, exact head SHA/tree, PRs, releases, packages, GitHub Pages, dependencies, branch/ruleset protections, Actions runners, secrets, App/webhook grants and rollback plan. Require explicit source and target administrators plus independent security reviewer. Never export/mirror private sources as a shortcut. After an actual GitHub ownership transfer, verify unchanged ID/history/tree, re-bind security controls and run exact-head CI. A job with `steps=[]` or `steps=null` is `NOT_EXECUTED`, and host/reference results do not prove compiled AttoLang guest or production HTTP execution.
