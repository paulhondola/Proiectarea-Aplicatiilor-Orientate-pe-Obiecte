#!/usr/bin/env bash
# Runs the real program the way a person would.
# Usage: test_cli.sh <paoo path> <sample.png> <output dir>
set -u

paoo=$1
sample=$2
out=$3/cli
mkdir -p "$out"
failures=0

fail() {
    echo "FAIL: $1"
    failures=$((failures + 1))
}

expect_error() { # <description> <arguments...>
    local description=$1
    shift
    if "$paoo" "$@" >"$out/stdout" 2>"$out/stderr"; then
        fail "$description: expected a non-zero exit"
    elif [ ! -s "$out/stderr" ]; then
        fail "$description: expected a message on stderr"
    fi
}

# A normal run writes the file and reports it.
if ! "$paoo" filter -i "$sample" -o "$out/out.png" -f grayscale -f blur:2 >"$out/stdout" 2>&1; then
    fail "filter run failed: $(cat "$out/stdout")"
fi
[ -s "$out/out.png" ] || fail "filter did not create its output"
grep -q "Wrote" "$out/stdout" || fail "filter did not report what it wrote"
[ "$(wc -l <"$out/stdout")" -eq 1 ] || fail "filter should print one line, with tracing off"

# The output can be read back by a later run.
"$paoo" filter -i "$out/out.png" -o "$out/again.jpg" -f invert >/dev/null 2>&1 ||
    fail "filter could not read its own output"

# No subcommand prints usage and fails.
if "$paoo" >"$out/stdout" 2>&1; then
    fail "no subcommand: expected a non-zero exit"
fi
grep -q "filter" "$out/stdout" || fail "no subcommand: expected usage text"

# Every error case.
expect_error "no filter" filter -i "$sample" -o "$out/error.png"
expect_error "no input" filter -o "$out/error.png" -f invert
expect_error "no output" filter -i "$sample" -f invert
expect_error "missing input" filter -i "$out/missing.png" -o "$out/error.png" -f invert
expect_error "bad radius" filter -i "$sample" -o "$out/error.png" -f blur:65
expect_error "unknown filter" filter -i "$sample" -o "$out/error.png" -f sharpen
expect_error "unsupported extension" filter -i "$sample" -o "$out/error.gif" -f invert
expect_error "missing output directory" filter -i "$sample" -o "$out/missing/out.png" -f invert

# A failed run must not touch an existing output, even if a later filter is the bad one.
cp "$out/out.png" "$out/existing.png"
expect_error "bad later filter" filter -i "$sample" -o "$out/existing.png" -f invert -f blur:65
cmp -s "$out/out.png" "$out/existing.png" || fail "a failed run changed an existing output"

# Input and output can be the same file.
cp "$sample" "$out/same.png"
"$paoo" filter -i "$out/same.png" -o "$out/same.png" -f invert >/dev/null 2>&1 ||
    fail "same input and output path failed"
"$paoo" filter -i "$out/same.png" -o "$out/same-check.png" -f invert >/dev/null 2>&1 ||
    fail "the rewritten file could not be read"

# Uppercase output extension.
"$paoo" filter -i "$sample" -o "$out/UPPER.PNG" -f invert >/dev/null 2>&1 ||
    fail "uppercase extension failed"

# A 4-channel input can be written as JPEG.
"$paoo" filter -i "$sample" -o "$out/rgba.jpg" -f invert >/dev/null 2>&1 ||
    fail "RGBA input to JPEG failed"

if [ "$failures" -ne 0 ]; then
    echo "$failures check(s) failed"
    exit 1
fi
echo "All CLI checks passed"
