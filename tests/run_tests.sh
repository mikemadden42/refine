#!/bin/sh
# Black-box tests for refine.
#
# Usage: run_tests.sh [path-to-refine-binary]
# Defaults to ./build/refine relative to the repo root.
#
# Each test builds a controlled fixture directory, runs the binary, and
# compares stdout and the exit code against expected values.

set -u

REFINE=${1:-}
if [ -z "$REFINE" ]; then
    script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
    REFINE="$script_dir/../build/refine"
fi

if [ ! -x "$REFINE" ]; then
    echo "error: refine binary not found or not executable: $REFINE" >&2
    echo "build it first, e.g.: cmake -S . -B build && cmake --build build" >&2
    exit 2
fi

tests_run=0
tests_failed=0

# check NAME EXPECTED_EXIT EXPECTED_STDOUT_FILE -- ARGS...
# Runs "$REFINE ARGS..." and compares exit code + stdout.
check() {
    name=$1
    expected_exit=$2
    expected_out=$3
    shift 4  # drop name, exit, out-file, and the literal "--"

    actual_out=$("$REFINE" "$@" 2>/dev/null)
    actual_exit=$?

    tests_run=$((tests_run + 1))
    ok=1

    if [ "$actual_exit" != "$expected_exit" ]; then
        ok=0
        printf 'FAIL %s: exit %s, expected %s\n' "$name" "$actual_exit" "$expected_exit"
    fi

    if [ "$actual_out" != "$(cat "$expected_out")" ]; then
        ok=0
        printf 'FAIL %s: stdout mismatch\n' "$name"
        printf -- '--- expected ---\n%s\n--- actual ---\n%s\n' \
            "$(cat "$expected_out")" "$actual_out"
    fi

    if [ "$ok" = 1 ]; then
        printf 'PASS %s\n' "$name"
    else
        tests_failed=$((tests_failed + 1))
    fi
}

# check_exit NAME EXPECTED_EXIT -- ARGS...
# Like check but only asserts the exit code (for help/error paths).
check_exit() {
    name=$1
    expected_exit=$2
    shift 3  # drop name, exit, and the literal "--"

    "$REFINE" "$@" >/dev/null 2>&1
    actual_exit=$?

    tests_run=$((tests_run + 1))
    if [ "$actual_exit" = "$expected_exit" ]; then
        printf 'PASS %s\n' "$name"
    else
        tests_failed=$((tests_failed + 1))
        printf 'FAIL %s: exit %s, expected %s\n' "$name" "$actual_exit" "$expected_exit"
    fi
}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# Fixture: a mix of extensions, a no-extension file, a dotfile, a subdir.
fixture="$work/fixture"
mkdir -p "$fixture/subdir"
touch "$fixture/b.txt" "$fixture/a.txt" "$fixture/c.md" \
      "$fixture/notes" "$fixture/.hidden" "$fixture/archive.tar.gz"

# Expected default output: dotfile excluded, subdir excluded (not regular),
# groups sorted by extension (No Extension first), names sorted within a group.
cat >"$work/expected_default" <<'EOF'
No Extension:
- notes

gz:
- archive.tar.gz

md:
- c.md

txt:
- a.txt
- b.txt

EOF

# With -a the dotfile appears under No Extension (leading dot is not an ext).
cat >"$work/expected_all" <<'EOF'
No Extension:
- .hidden
- notes

gz:
- archive.tar.gz

md:
- c.md

txt:
- a.txt
- b.txt

EOF

# Empty directory produces no output.
mkdir "$work/empty"
: >"$work/expected_empty"

check "default grouping"   0 "$work/expected_default" -- "$fixture"
check "include hidden -a"  0 "$work/expected_all"     -- -a "$fixture"
check "include hidden --all" 0 "$work/expected_all"   -- --all "$fixture"
check "empty directory"    0 "$work/expected_empty"   -- "$work/empty"

check_exit "help -h"            0 -- -h
check_exit "help --help"        0 -- --help
check_exit "nonexistent path"   1 -- "$work/does-not-exist"
check_exit "not a directory"    1 -- "$fixture/a.txt"
check_exit "unknown option"     2 -- --bogus
check_exit "extra argument"     2 -- "$fixture" "$fixture"

echo
printf '%s test(s) run, %s failed\n' "$tests_run" "$tests_failed"
[ "$tests_failed" = 0 ]
