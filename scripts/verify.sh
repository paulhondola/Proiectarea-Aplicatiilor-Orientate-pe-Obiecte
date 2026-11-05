#!/usr/bin/env bash
# The one definition of the project's checks. lefthook and CI call these steps.
# Usage: scripts/verify.sh [step]   (no step runs them all, stopping at the first failure)
set -euo pipefail
cd "$(dirname "$0")/.."

# clang-format and clang-tidy come from Homebrew's llvm, which is not on PATH by default.
if command -v brew >/dev/null && [ -d "$(brew --prefix llvm)/bin" ]; then
    PATH="$(brew --prefix llvm)/bin:$PATH"
fi

# Project files only: third_party/ and build/ are excluded by listing these paths.
project_files() { # <git pathspecs...>
    git ls-files --cached --others --exclude-standard -- "$@" | while read -r file; do
        [ -f "$file" ] && echo "$file"
    done
}

# One build directory per build system, reconfigured by each step: build/ for CMake, target/ for Meson.
# Every option is passed on each configure so nothing leaks from the previous step.
# Each check records its demo output in build/parity/ for the parity step.
cmake_check() { # <label> <build type> <sanitize ON|OFF>
    cmake -S . -B build -DCMAKE_BUILD_TYPE="$2" -DPAOO_SANITIZE="$3"
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    record_demo "$1" build/src/paoo
}

meson_check() { # <label> <meson args...>
    local label=$1
    shift
    meson setup --wipe target "$@" 2>/dev/null || meson setup target "$@"
    meson compile -C target
    meson test -C target --suite PAOO --print-errorlogs
    record_demo "$label" target/src/paoo
}

record_demo() { # <label> <paoo binary>
    mkdir -p build/parity
    "$2" demo | grep '^\[PASS\]' | sort >"build/parity/$1"
}

step_format() {
    project_files 'inc/*.hpp' 'src/*.cpp' 'tests/*.cpp' | xargs clang-format --dry-run --Werror
}
step_lint() {
    # Lint only needs the compile database.
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPAOO_SANITIZE=OFF >/dev/null
    # stb_impl.cpp only compiles third-party code.
    project_files 'src/*.cpp' 'tests/*.cpp' | grep -v 'src/stb_impl.cpp' |
        xargs -P 8 -n 1 clang-tidy -p build --quiet --warnings-as-errors='*' \
            "--extra-arg=-isysroot$(xcrun --show-sdk-path)" --extra-arg=-Wno-c2y-extensions
}
step_cmake_debug() { cmake_check cmake-debug Debug OFF; }
step_cmake_release() { cmake_check cmake-release Release OFF; }
step_meson_debug() { meson_check meson-debug --buildtype=debug; }
# b_ndebug makes Meson's Release define NDEBUG as CMake's does.
step_meson_release() { meson_check meson-release --buildtype=release -Db_ndebug=if-release; }
step_sanitize() {
    cmake_check cmake-sanitize Debug ON
    # b_lundef must be off for sanitizers under Clang.
    meson_check meson-sanitize --buildtype=debug \
        -Db_sanitize=address,undefined -Db_lundef=false \
        -Dcpp_args=-fno-sanitize-recover=undefined -Dcpp_link_args=-fno-sanitize-recover=undefined
}

step_parity() {
    # Compares the demo checks recorded by the other steps; run those first.
    for label in cmake-release meson-debug meson-release; do
        if ! diff -q build/parity/cmake-debug "build/parity/$label" >/dev/null; then
            echo "$label printed different checks from cmake-debug" >&2
            return 1
        fi
    done
}
step_leaks() {
    # Needs a build without sanitizers: leaks cannot inspect ASan-instrumented programs.
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPAOO_SANITIZE=OFF >/dev/null
    cmake --build build --parallel
    leaks --atExit -- build/src/paoo demo >build/leaks.log 2>&1 || {
        cat build/leaks.log
        return 1
    }
    leaks --atExit -- build/src/paoo filter -i tests/data/sample.png \
        -o build/leaks-out.png -f grayscale -f blur:2 >build/leaks.log 2>&1 || {
        cat build/leaks.log
        return 1
    }
}

ALL_STEPS="format lint cmake-debug cmake-release meson-debug meson-release parity sanitize leaks"

run_step() {
    echo "==> $1"
    case "$1" in
    format) step_format ;;
    lint) step_lint ;;
    cmake-debug) step_cmake_debug ;;
    cmake-release) step_cmake_release ;;
    meson-debug) step_meson_debug ;;
    meson-release) step_meson_release ;;
    parity) step_parity ;;
    sanitize) step_sanitize ;;
    leaks) step_leaks ;;
    *)
        echo "Unknown step: $1 (steps: $ALL_STEPS)" >&2
        exit 2
        ;;
    esac
}

if [ $# -gt 1 ]; then
    echo "Usage: $0 [step]" >&2
    exit 2
fi

for step in ${1:-$ALL_STEPS}; do
    run_step "$step"
done
echo "OK: ${1:-all steps}"
