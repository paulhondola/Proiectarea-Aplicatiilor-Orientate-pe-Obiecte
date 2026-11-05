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

cmake_check() { # <build dir> <cmake args...>
    local dir=$1
    shift
    cmake -S . -B "$dir" "$@"
    cmake --build "$dir" --parallel
    ctest --test-dir "$dir" --output-on-failure
}

meson_check() { # <build dir> <meson args...>
    local dir=$1
    shift
    if [ -d "$dir/meson-info" ]; then
        meson setup --reconfigure "$dir" "$@"
    else
        meson setup "$dir" "$@"
    fi
    meson compile -C "$dir"
    meson test -C "$dir" --suite PAOO --print-errorlogs
}

step_format() {
    project_files 'inc/*.hpp' 'src/*.cpp' 'tests/*.cpp' | xargs clang-format --dry-run --Werror
}
step_lint() {
    # Lint runs first in the full order, and only needs the compile database.
    cmake -S . -B build/cmake-debug -DCMAKE_BUILD_TYPE=Debug >/dev/null
    # stb_impl.cpp only compiles third-party code.
    project_files 'src/*.cpp' 'tests/*.cpp' | grep -v 'src/stb_impl.cpp' |
        xargs -P 8 -n 1 clang-tidy -p build/cmake-debug --quiet --warnings-as-errors='*' \
            "--extra-arg=-isysroot$(xcrun --show-sdk-path)" --extra-arg=-Wno-c2y-extensions
}
step_cmake_debug() { cmake_check build/cmake-debug -DCMAKE_BUILD_TYPE=Debug; }
step_cmake_release() { cmake_check build/cmake-release -DCMAKE_BUILD_TYPE=Release; }
step_meson_debug() { meson_check build/meson-debug --buildtype=debug; }
# b_ndebug makes Meson's Release define NDEBUG as CMake's does.
step_meson_release() { meson_check build/meson-release --buildtype=release -Db_ndebug=if-release; }
step_sanitize() {
    cmake_check build/cmake-sanitize -DCMAKE_BUILD_TYPE=Debug -DPAOO_SANITIZE=ON
    # b_lundef must be off for sanitizers under Clang.
    meson_check build/meson-sanitize --buildtype=debug \
        -Db_sanitize=address,undefined -Db_lundef=false \
        -Dcpp_args=-fno-sanitize-recover=undefined -Dcpp_link_args=-fno-sanitize-recover=undefined
}

step_parity() {
    local expected
    expected=$(build/cmake-debug/src/paoo demo | grep '^\[PASS\]' | sort)
    for dir in build/cmake-release build/meson-debug build/meson-release; do
        if [ "$("$dir/src/paoo" demo | grep '^\[PASS\]' | sort)" != "$expected" ]; then
            echo "$dir printed different checks from build/cmake-debug" >&2
            return 1
        fi
    done
}
step_leaks() {
    # Needs a build without sanitizers: leaks cannot inspect ASan-instrumented programs.
    leaks --atExit -- build/cmake-debug/src/paoo demo >build/leaks.log 2>&1 || {
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
