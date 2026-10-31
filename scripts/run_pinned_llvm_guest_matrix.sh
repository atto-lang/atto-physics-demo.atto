#!/usr/bin/env bash
# Executes only real compiler-generated Oreslang guest objects. Linux/sha256sum gate.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
: "${EXPECTED_PHYSICS_COMMIT_SHA:?Set expected full physics Git commit SHA}"
: "${EXPECTED_PHYSICS_TREE_SHA:?Set independently verified full source Git tree SHA}"
: "${EXPECTED_LLVM_COMPILER_SHA256:?Set independently approved compiler binary SHA256}"
: "${EXPECTED_CLANG_SHA256:?Set independently approved Clang binary SHA256}"
: "${ORESLANG_LLVM_COMPILER:?Set absolute path to genuine oreslang-llvmc}"
: "${ORE_CLANG:?Set absolute path to genuine Clang executable}"

require_sha() {
  [[ "$2" =~ ^[0-9a-f]{${3}}$ ]] || { echo "Invalid $1" >&2; exit 2; }
}
require_sha EXPECTED_PHYSICS_COMMIT_SHA "$EXPECTED_PHYSICS_COMMIT_SHA" 40
require_sha EXPECTED_PHYSICS_TREE_SHA "$EXPECTED_PHYSICS_TREE_SHA" 40
require_sha EXPECTED_LLVM_COMPILER_SHA256 "$EXPECTED_LLVM_COMPILER_SHA256" 64
require_sha EXPECTED_CLANG_SHA256 "$EXPECTED_CLANG_SHA256" 64
for binary in "$ORESLANG_LLVM_COMPILER" "$ORE_CLANG"; do
  [[ "$binary" = /* && -f "$binary" && -x "$binary" ]] || {
    echo "Compiler path must be absolute and executable: $binary" >&2; exit 2;
  }
done
command -v sha256sum >/dev/null || { echo 'sha256sum required' >&2; exit 2; }
source_head=$(git -C "$root" rev-parse HEAD)
[[ "$source_head" == "$EXPECTED_PHYSICS_COMMIT_SHA" ]] || {
  echo "Physics source HEAD mismatch: $source_head" >&2; exit 2;
}
source_tree=$(git -C "$root" rev-parse 'HEAD^{tree}')
[[ "$source_tree" == "$EXPECTED_PHYSICS_TREE_SHA" ]] || {
  echo "Physics source tree mismatch: $source_tree" >&2; exit 2;
}
[[ -z "$(git -C "$root" status --porcelain --untracked-files=all)" ]] || {
  echo 'Physics source worktree is not clean' >&2; exit 2;
}
compiler_sha=$(sha256sum "$ORESLANG_LLVM_COMPILER" | cut -d' ' -f1)
clang_sha=$(sha256sum "$ORE_CLANG" | cut -d' ' -f1)
[[ "$compiler_sha" == "$EXPECTED_LLVM_COMPILER_SHA256" ]] || {
  echo "Oreslang compiler digest mismatch" >&2; exit 2;
}
[[ "$clang_sha" == "$EXPECTED_CLANG_SHA256" ]] || {
  echo "Clang digest mismatch" >&2; exit 2;
}
printf 'PHYSICS_GUEST_GATE_SOURCE_COMMIT=%s\n' "$source_head"
printf 'PHYSICS_GUEST_GATE_SOURCE_TREE=%s\n' "$source_tree"
printf 'PHYSICS_GUEST_GATE_COMPILER_SHA256=%s\n' "$compiler_sha"
printf 'PHYSICS_GUEST_GATE_CLANG_SHA256=%s\n' "$clang_sha"
"$ORE_CLANG" --version | head -n 1

# Build directories are intentionally recreated: a matching Git commit plus a
# newly pinned compiler must never reuse IR/object/executable from an earlier
# invocation. Refuse symlinked build roots before deleting fixed subpaths.
if [[ -L "$root/build" ]]; then
  echo "Refusing symlinked build root" >&2; exit 2;
fi
mkdir -p -- "$root/build"
prior_digest=''
for level in 0 2; do
  build="$root/build/pinned-native-guest-O${level}"
  if [[ -L "$build" ]]; then
    echo "Refusing symlinked guest build directory: $build" >&2; exit 2;
  fi
  rm -rf -- "$build"
  cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release \
    -DORES_PHYSICS_GUI=OFF -DORES_PHYSICS_QUEUE_ONLY=OFF \
    "-DORES_PHYSICS_GUEST_OPT_LEVEL=${level}" \
    "-DORESLANG_LLVM_COMPILER=${ORESLANG_LLVM_COMPILER}" \
    "-DORE_CLANG=${ORE_CLANG}"
  cmake --build "$build" --parallel 2
  ctest --test-dir "$build" --output-on-failure
  output=$("$build/physics_native_contract")
  grep -qx 'REAL_ORESLANG_LLVM_PHYSICS_CONTRACT_PASS' <<< "$output" || {
    echo "O${level} executable did not emit guest PASS marker" >&2; exit 1;
  }
  digest=$(sed -n 's/^PHYSICS_GUEST_1200_TICK_DIGEST=\([0-9a-f]\{16\}\)$/\1/p' <<< "$output")
  [[ "$digest" =~ ^[0-9a-f]{16}$ ]] || {
    echo "O${level} did not emit one valid 1200-tick digest" >&2; exit 1;
  }
  echo "PHYSICS_O${level}_GUEST_REPLAY_DIGEST=$digest"
  sha256sum "$build/native-physics.ll" "$build/native-physics.o" \
    "$build/physics_native_contract"
  if [[ -n "$prior_digest" && "$prior_digest" != "$digest" ]]; then
    echo 'O0/O2 guest replay divergence' >&2; exit 1;
  fi
  prior_digest="$digest"
done
echo "REAL_ORESLANG_LLVM_PHYSICS_O0_O2_GUEST_GATE_PASS"
