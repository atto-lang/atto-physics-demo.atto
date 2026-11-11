#!/usr/bin/env bash
# Build and exercise an exact source snapshot with the real compiler.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
: "${ORESLANG_LLVM_COMPILER:?Absolute genuine compiler path required}"
: "${ORE_CLANG:?Absolute Clang path required}"
: "${QT_PREFIX:?Qt installation prefix required}"
export EXPECTED_PHYSICS_COMMIT_SHA=${EXPECTED_PHYSICS_COMMIT_SHA:-$(git -C "$root" rev-parse HEAD)}
export EXPECTED_PHYSICS_TREE_SHA=${EXPECTED_PHYSICS_TREE_SHA:-$(git -C "$root" rev-parse 'HEAD^{tree}')}
export EXPECTED_LLVM_COMPILER_SHA256=${EXPECTED_LLVM_COMPILER_SHA256:-$(sha256sum "$ORESLANG_LLVM_COMPILER" | cut -d' ' -f1)}
export EXPECTED_CLANG_SHA256=${EXPECTED_CLANG_SHA256:-$(sha256sum "$ORE_CLANG" | cut -d' ' -f1)}
bash "$root/scripts/run_pinned_llvm_guest_matrix.sh"
build=$(mktemp -d "$root/build/packaged-qt.XXXXXXXX")
printf 'PACKAGED_QT_BUILD=%s\n' "$build"
cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release \
  "-DCMAKE_PREFIX_PATH=$QT_PREFIX" "-DQt6_DIR=$QT_PREFIX/lib/cmake/Qt6" \
  -DORES_PHYSICS_GUI_PROOF=ON \
  "-DORESLANG_LLVM_COMPILER=$ORESLANG_LLVM_COMPILER" "-DORE_CLANG=$ORE_CLANG"
cmake --build "$build" --parallel 4
ctest --test-dir "$build" --output-on-failure --no-tests=error
"$QT_PREFIX/bin/macdeployqt" "$build/oreslang_physics.app" "-qmldir=$root/ui" "-libpath=$QT_PREFIX/lib" -always-overwrite -no-strip -no-codesign
# One final ad-hoc signature after all dependency rewrites.
codesign --force --deep --sign - "$build/oreslang_physics.app"
# No Qt environment search paths; load the frameworks/QML/plugins in the bundle.
env -u QML_IMPORT_PATH -u QML2_IMPORT_PATH -u QT_PLUGIN_PATH \
  "ATTO_PHYSICS_GUI_PROOF=$build/evidence" \
  "$build/oreslang_physics.app/Contents/MacOS/oreslang_physics"
ditto -c -k --sequesterRsrc --keepParent "$build/oreslang_physics.app" "$build/AttoLang-Physics-macOS.zip"
sha256sum "$build/AttoLang-Physics-macOS.zip" "$build/native-physics.ll" \
  "$build/native-physics.o" "$build/oreslang_physics.app/Contents/MacOS/oreslang_physics"
echo MACOS_PACKAGED_REAL_GUEST_PROOF_PASS
