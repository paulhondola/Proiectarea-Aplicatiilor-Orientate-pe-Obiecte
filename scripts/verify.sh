#!/usr/bin/env bash
# The one definition of the project's checks. lefthook and CI call these steps.
# Usage: scripts/verify.sh [step]   (no step runs them all, stopping at the first failure)
set -euo pipefail
cd "$(dirname "$0")/.."

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

ALL_STEPS="cmake-debug cmake-release meson-debug meson-release sanitize"

run_step() {
    echo "==> $1"
    case "$1" in
    cmake-debug) step_cmake_debug ;;
    cmake-release) step_cmake_release ;;
    meson-debug) step_meson_debug ;;
    meson-release) step_meson_release ;;
    sanitize) step_sanitize ;;
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
