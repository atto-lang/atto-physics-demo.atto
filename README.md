# Oreslang Physics Demo (LLVM + Qt Quick)

A native desktop physics playground built from Oreslang source compiled through LLVM, with a Qt Quick/QML interface.

**Status:** repository initialized; native simulation, host adapter, conformance tests and QML frontend are being staged on a feature branch. A successful visual demo or hard real-time guarantee has not yet been verified.

Design constraints: no actors required for physics; fixed-step simulation; bounded preallocated state; zero tracing GC in the simulation; thin native Qt rendering bridge. The GUI is explicitly **not** a hard-real-time component.

See the feature branch / pull request for the implementation and executable verification gates.
