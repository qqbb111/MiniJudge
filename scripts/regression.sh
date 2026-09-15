#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MINIJUDGE="$ROOT_DIR/build/minijudge"
EXAMPLES="$ROOT_DIR/examples"

PASS_COUNT=0
FAIL_COUNT=0

if [[ ! -x "$MINIJUDGE" ]]; then
    echo "MiniJudge executable not found: $MINIJUDGE"
    echo "Run: cmake --build build"
    exit 1
fi

print_pass() {
    local name="$1"
    printf "%-24s PASS\n" "$name"
    PASS_COUNT=$((PASS_COUNT + 1))
}

print_fail() {
    local name="$1"
    local output="$2"

    printf "%-24s FAIL\n" "$name"
    echo "----- output -----"
    echo "$output"
    echo "------------------"

    FAIL_COUNT=$((FAIL_COUNT + 1))
}

# 检查运行期 verdict：AC / WA / RE / TLE / MLE
#
# 要求：
# 1. 至少出现一个 Test 行
# 2. 所有 Test 行都是 expected verdict
run_verdict_test() {
    local name="$1"
    local source="$2"
    local expected="$3"
    shift 3

    local output
    output="$("$MINIJUDGE" "$source" "$@" 2>&1)"

    local test_lines
    test_lines="$(printf '%s\n' "$output" | grep '^Test ' || true)"

    if [[ -z "$test_lines" ]]; then
        print_fail "$name" "$output"
        return
    fi

    if ! printf '%s\n' "$test_lines" |
        grep -Eq "^Test .*: ${expected}( \(|$)"; then
        print_fail "$name" "$output"
        return
    fi

    if printf '%s\n' "$test_lines" |
        grep -Evq "^Test .*: ${expected}( \(|$)"; then
        print_fail "$name" "$output"
        return
    fi

    print_pass "$name"
}

# CE 没有 Test 行，单独检查
run_ce_test() {
    local output
    output="$("$MINIJUDGE" "$EXAMPLES/ce.cpp" -t 1000 -m 64 2>&1)"

    if printf '%s\n' "$output" | grep -Eq ' CE$'; then
        print_pass "Compile Error"
    else
        print_fail "Compile Error" "$output"
    fi
}

echo "MiniJudge regression tests"
echo "=========================="

run_verdict_test \
    "Accepted" \
    "$EXAMPLES/ac.cpp" \
    "AC" \
    -t 1000 -m 64

run_verdict_test \
    "Wrong Answer" \
    "$EXAMPLES/wa.cpp" \
    "WA" \
    -t 1000 -m 64

run_ce_test

run_verdict_test \
    "Runtime Error" \
    "$EXAMPLES/re_return1.cpp" \
    "RE" \
    -t 1000 -m 64

run_verdict_test \
    "Core Dump RE" \
    "$EXAMPLES/re_coredumping1.cpp" \
    "RE" \
    -t 1000 -m 64

run_verdict_test \
    "CPU Time Limit" \
    "$EXAMPLES/tle.cpp" \
    "TLE" \
    -t 1000 -m 64

run_verdict_test \
    "Wall Time Watchdog" \
    "$EXAMPLES/sleep.cpp" \
    "TLE" \
    -t 1000 -m 64

run_verdict_test \
    "Memory Limit" \
    "$EXAMPLES/mle_128MB.cpp" \
    "MLE" \
    -t 1000 -m 64

echo
echo "=========================="
echo "$PASS_COUNT passed, $FAIL_COUNT failed"

if [[ "$FAIL_COUNT" -ne 0 ]]; then
    exit 1
fi

exit 0
