#!/usr/bin/env bash
# Negative-only admission checks. Synthetic programs are NEVER invoked to
# produce LLVM IR, and no physics guest or compiler is executed.
set -euo pipefail
source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
runner="$source_root/scripts/run_pinned_llvm_guest_matrix.sh"
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
repo="$work/fixture"
mkdir -p "$repo/scripts"
cp -- "$runner" "$repo/scripts/run_pinned_llvm_guest_matrix.sh"
# Ignore both directory and symlink forms for the symlinked-root negative case.
printf 'build\n' > "$repo/.gitignore"
git -C "$repo" init -q
git -C "$repo" add .
git -C "$repo" -c user.name=PhysicsContract -c user.email=contract@example.invalid commit -qm fixture
head=$(git -C "$repo" rev-parse HEAD)
tree=$(git -C "$repo" rev-parse 'HEAD^{tree}')

# The executable is a sentinel. Only `--version` is permitted. Any attempt
# to use it as a guest compiler/Clang build is recorded and fails the test.
tool="$work/sentinel-tool"
cat > "$tool" <<'SENTINEL'
#!/bin/sh
if [ "${1:-}" = '--version' ]; then
  printf 'sentinel version (negative-only preflight)\n'
  exit 0
fi
printf 'NOT_AN_ORESLANG_COMPILER\n' > "$PHYSICS_SENTINEL_TRIGGER"
exit 71
SENTINEL
chmod +x "$tool"
sha=$(sha256sum "$tool" | cut -d' ' -f1)
trigger="$work/compiler-was-executed"
wrong_sha=0000000000000000000000000000000000000000000000000000000000000000
wrong_tree=0000000000000000000000000000000000000000
run_rejected() {
  local label="$1" expected="$2" pinned_head="$3" pinned_tree="$4" pinned_compiler="$5" pinned_clang="$6"
  local output
  if output=$(env \
      EXPECTED_PHYSICS_COMMIT_SHA="$pinned_head" \
      EXPECTED_PHYSICS_TREE_SHA="$pinned_tree" \
      EXPECTED_LLVM_COMPILER_SHA256="$pinned_compiler" \
      EXPECTED_CLANG_SHA256="$pinned_clang" \
      ORESLANG_LLVM_COMPILER="$tool" \
      ORE_CLANG="$tool" \
      PHYSICS_SENTINEL_TRIGGER="$trigger" \
      bash "$repo/scripts/run_pinned_llvm_guest_matrix.sh" 2>&1); then
    printf '%s unexpectedly admitted\n' "$label" >&2
    exit 1
  fi
  if ! grep -Fq -- "$expected" <<< "$output"; then
    printf '%s rejected for wrong reason:\n%s\n' "$label" "$output" >&2
    exit 1
  fi
  if [ -e "$trigger" ]; then
    printf '%s ran a synthetic compiler instead of failing pre-admission\n' "$label" >&2
    exit 1
  fi
}
run_rejected 'wrong head' 'Physics source HEAD mismatch' "$wrong_tree" "$tree" "$sha" "$sha"
run_rejected 'wrong tree' 'Physics source tree mismatch' "$head" "$wrong_tree" "$sha" "$sha"
printf 'dirty\n' > "$repo/dirty.txt"
run_rejected 'dirty source tree' 'worktree is not clean' "$head" "$tree" "$sha" "$sha"
rm -- "$repo/dirty.txt"
run_rejected 'compiler digest mismatch' 'Oreslang compiler digest mismatch' "$head" "$tree" "$wrong_sha" "$sha"
run_rejected 'Clang digest mismatch' 'Clang digest mismatch' "$head" "$tree" "$sha" "$wrong_sha"
mkdir -p "$work/target"
printf 'keep me\n' > "$work/target/marker"
ln -s "$work/target" "$repo/build"
run_rejected 'symlinked build root' 'Refusing symlinked build root' "$head" "$tree" "$sha" "$sha"
test -f "$work/target/marker"
if grep -Eq '(^|[[:space:]])rm[[:space:]]+-rf([[:space:]]|$)' "$runner"; then
  echo "Pinned guest matrix must not recursively delete previous build artifacts" >&2
  exit 1
fi
echo 'PINNED_GUEST_ADMISSION_NO_GUEST_CONTRACT_PASS'
