# Image Filter Pipeline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace `SensorLogger` with a CLI image filter tool whose `demo` command covers every graded item in Lucrarea 3, 4 and 5, built and checked under both CMake and Meson.

**Architecture:** A static library `image_lib` holds `Image`, `LabeledImage`, `Logger`, `Filters` and `ImageIO`. The `paoo` executable holds `main.cpp` and the lab demos. `scripts/verify.sh` defines every check once, and lefthook and CI call it.

**Tech Stack:** C++26 (C++23 fallback), CMake >= 3.25, Meson >= 1.3, CLI11 v2.7.2, Catch2 v3.16.1, stb (vendored), Homebrew LLVM (`clang-format`, `clang-tidy`), lefthook, GitHub Actions on `macos-latest`, `leaks`.

**Spec:** `docs/2026-11-05-image-filter-pipeline-design.md`. Read it before starting. Section numbers below refer to it.

## Global Constraints

- C++26 with compiler extensions off, falling back to C++23 when the compiler doesn't offer it. If a pinned library doesn't build under C++26, try a newer release of it, then move the project to C++23, and tell Paul.
- The executable is `paoo`, written literally in both builds. The project name stays `PAOO`, so `add_executable(${PROJECT_NAME} ...)` must go.
- CLI11 v2.7.2 (wrap `2.7.2-1`), Catch2 v3.16.1 (wrap `3.16.1-1`), stb commit `d14d46f6b44456eb88beac58cba833a4135d4c89`.
- Existing style: `inc/`, `src/`, `.hpp`, `#ifndef` guards, four-space indentation, camelCase methods, `getX()` accessors, members without prefixes or suffixes.
- Project code uses `-Wall -Wextra -Wpedantic` without `-Werror`. Third-party code produces no warnings.
- CMake and Meson have the same targets, tests and source order. Every task that adds a source or test updates both.
- Demos contain no undefined behaviour and no leaks. Their checks use `check()`, never `assert`.
- All visible text is in English.
- `scripts/verify.sh` passes at the end of each lab's commits (Tasks 4, 5 and 7, and again at the end).
- Don't push. Pushing is Paul's decision (Task 10).

## Decisions This Plan Adds

- **`filter` arrives last.** It needs `Image::operator=`, which belongs to Lucrarea 5, so the Lucrarea 3 and 4 checkpoints only have the `demo` command.
- **`stb_impl.cpp` gets its own target.** `stb_impl` compiles it with `-w`, which is the simplest way to silence stb identically in both builds. `image_lib` links it.
- **`verify.sh parity` is added** to prove the spec's Debug and Release demo requirement (section 11).
- **`Logger` starts disabled.** Only `demo` turns it on.

## Review Focus

- `filter` with the same input and output path should succeed, because the image is fully in memory before writing. (Task 9)
- A 4-channel image saved as `.jpg` should produce a valid 3-channel JPEG instead of failing. (Task 9)
- Uppercase extensions such as `OUT.PNG` should work like lowercase ones. (Task 9)
- Malformed filters (`blur:`, `blur:abc`, `blur:2x`, `blur:0`, `blur:65`, `grayscale:3`) should fail and leave an existing output file untouched. (Tasks 8 and 9)
- `blur:64` on a 1×1 image should return the same pixel. (Task 8)

---

### Task 1: Build skeleton and library proof

**Files:**
- Create: `third_party/stb/{stb_image.h,stb_image_write.h,LICENSE}`, `third_party/CMakeLists.txt`, `third_party/meson.build`
- Create: `src/CMakeLists.txt`, `src/meson.build`, `tests/CMakeLists.txt`, `tests/meson.build`, `subprojects/cli11.wrap`, `subprojects/catch2.wrap`
- Create: `inc/Logger.hpp`, `src/Logger.cpp`, `src/stb_impl.cpp`, `tests/test_logger.cpp`, `tests/test_libraries.cpp`, `scripts/verify.sh`
- Modify: `CMakeLists.txt`, `meson.build`, `.gitignore`, `src/main.cpp`
- Delete: `inc/SensorLogger.hpp`, `src/SensorLogger.cpp`

**Interfaces:**
- Produces: targets `image_lib`, `stb_impl`, `paoo`, `paoo_tests`. `Logger` exactly as in spec section 7. `verify.sh <step>` with steps `cmake-debug`, `cmake-release`, `meson-debug`, `meson-release`, `sanitize`.
- Build directories: `build/cmake-debug`, `build/cmake-release`, `build/cmake-sanitize`, `build/meson-debug`, `build/meson-release`, `build/meson-sanitize`. The executable is `<dir>/src/paoo`.

- [ ] **Step 1: Preserve the old code**

Run: `git tag lab3-sensor-logger f097781`
Expected: `git tag --list` shows the tag. Do not push it.

- [ ] **Step 2: Vendor stb**

Download `stb_image.h`, `stb_image_write.h` and `LICENSE` from `https://raw.githubusercontent.com/nothings/stb/d14d46f6b44456eb88beac58cba833a4135d4c89/` into `third_party/stb/`.

- [ ] **Step 3: Write both builds**

Root files set the standard and warnings. They also add `third_party`, `src` and `tests`, in that order.

CMake needs these:
- Minimum version 3.25. Choose 26 when `cxx_std_26` is in `CMAKE_CXX_COMPILE_FEATURES`, otherwise 23, with `CMAKE_CXX_STANDARD_REQUIRED ON` and `CMAKE_CXX_EXTENSIONS OFF`.
- `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` and `enable_testing()`.
- `option(PAOO_SANITIZE ...)`. When on, it adds `-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer` to compile and link options, **before** `FetchContent_MakeAvailable`.
- An INTERFACE target `paoo_warnings`, linked privately by project targets only.

Meson needs these:
- `warning_level=3`, `cpp_std=c++26,c++23` and `meson_version: '>= 1.3.0'`.
- `meson wrap install cli11` and `meson wrap install catch2` to produce the committed `.wrap` files.
- `dependency('CLI11')` and `dependency('catch2-with-main')`.
- `.gitignore` entries for `subprojects/*/` and `subprojects/packagecache/`.

- [ ] **Step 4: Write the failing tests**

```cpp
// tests/test_logger.cpp
static_assert(!std::is_copy_constructible<Logger>::value, "");
static_assert(!std::is_copy_assignable<Logger>::value, "");
TEST_CASE("Logger is one instance") {
    REQUIRE(&Logger::instance() == &Logger::instance());
}
TEST_CASE("Logger prints only when enabled") {
    // Redirect std::cout through rdbuf, and restore it and enabled=false afterwards.
    // Disabled: trace("x") prints nothing. Enabled: the output contains "x".
}
// tests/test_libraries.cpp
TEST_CASE("stb writes and reads a PNG") {
    // Write a 4x4x3 buffer of 200s to a file in the working directory.
    // stbi_load returns w==4, h==4, c==3, data[0]==200. Free it and remove the file.
}
```

- [ ] **Step 5: Run to verify it fails**

Run: `cmake -S . -B build/cmake-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/cmake-debug`
Expected: FAIL because `Logger.hpp` doesn't exist.

- [ ] **Step 6: Implement `Logger`, `stb_impl.cpp`, a minimal `main.cpp` and `verify.sh`**

`main.cpp` configures a CLI11 `App` with no subcommands yet and returns 0, which proves CLI11 links. `verify.sh` starts with `set -euo pipefail`, runs from the repo root, and dispatches on `$1`. With no argument it runs every step in order.

The Meson `sanitize` configuration uses `-Db_sanitize=address,undefined -Db_lundef=false -Dcpp_args=-fno-sanitize-recover=undefined -Dcpp_link_args=-fno-sanitize-recover=undefined`.

- [ ] **Step 7: Verify every configuration**

Run: `scripts/verify.sh`
Expected: exit 0, and both builds report `paoo_tests` passing under `-std=c++26`. This is the proof required by spec section 12. If a library fails on C++26, apply the Global Constraints rule and update the spec's version table.

- [ ] **Step 8: Commit**

Commit the removal, the build and `Logger` with the message `Replace SensorLogger build with CMake and Meson skeleton`.

---

### Task 2: `Image` lifetime and encapsulation

**Files:**
- Create: `inc/Image.hpp`, `src/Image.cpp`, `tests/test_image.cpp`
- Modify: `src/CMakeLists.txt`, `src/meson.build`, `tests/CMakeLists.txt`, `tests/meson.build`

**Interfaces:**
- Consumes: `Logger::instance().trace(const std::string&)`.
- Produces: `Image` as in spec section 7, without `operator=` yet. Keep `cloneBuffer` private and returning `nullptr` for empty sources.

- [ ] **Step 1: Write the failing tests**

```cpp
static_assert(std::is_nothrow_move_constructible<Image>::value, "");
TEST_CASE("default image is empty") {
    // width, height, channels and byte count are 0; isEmpty() is true; getData() == nullptr.
}
TEST_CASE("new image is zero-filled") {
    Image image(2, 3, 3);
    // getByteCount() == 18, and every at(x, y, c) == 0.
}
TEST_CASE("invalid dimensions throw") {
    // Each of (0,1,1), (1,0,1), (-1,1,1), (1,-1,1), (1,1,0), (1,1,5) throws std::invalid_argument.
}
TEST_CASE("at and set reject bad coordinates") {
    Image image(2, 2, 1);
    // x=-1, x=2, y=-1, y=2, c=-1 and c=1 throw std::out_of_range from both at and set.
    // After the rejected calls, every pixel is still 0.
}
TEST_CASE("copy is deep") {
    // Addresses differ and bytes match. set() on the copy leaves the original unchanged.
}
TEST_CASE("copy of empty is empty") {
    // getData() == nullptr.
}
TEST_CASE("move takes the buffer") {
    // The new object has the original address and pixels. The source isEmpty() with getData() == nullptr.
}
```

The overflow guard from spec section 7 has no runtime test. On 64-bit, the largest `int` dimensions don't overflow `std::size_t`, and an oversized allocation would abort under ASan. Implement the check by comparing against `std::numeric_limits<std::size_t>::max()` before multiplying.

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL because `Image.hpp` doesn't exist.

- [ ] **Step 3: Implement `Image` in `src/Image.cpp`**

Allocate zeroed pixels with `new unsigned char[n]()`. The initializer list relies on `pixels` being declared after the dimensions. Each special member traces its action and buffer address through `Logger`. Copies must not alter any data.

- [ ] **Step 4: Run to verify it passes**

Run: `scripts/verify.sh cmake-debug && scripts/verify.sh meson-debug`
Expected: all tests pass.

- [ ] **Step 5: Commit**

Message: `Add Image with deep copy and move`.

---

### Task 3: Lucrarea 3 demo and the `demo` command

**Files:**
- Create: `inc/Demos.hpp`, `inc/DemoTypes.hpp`, `src/demos/lab3.cpp`
- Modify: `src/main.cpp`, `src/CMakeLists.txt`, `src/meson.build`, `tests/CMakeLists.txt`, `tests/meson.build`

**Interfaces:**
- Produces in `Demos.hpp`:
    - `void check(bool condition, const std::string& description);`. It prints `[PASS] <description>` or throws `std::runtime_error`.
    - `void runLab3();`
- Produces in `DemoTypes.hpp`:
    - `struct ShallowImage`, with public `unsigned char* data` that its destructor frees, and the generated copy constructor.
    - `class FilterPreset { public: FilterPreset(const std::string& name, int radius); const std::string name; const int radius; };`
- Produces in `main.cpp`: `paoo demo [--lab N]`, with `N` limited to labs that exist (extend the range in Tasks 5 and 6). No subcommand prints help and returns 1. Catch `std::exception`, print to `stderr`, return 1. `demo` calls `Logger::instance().setEnabled(true)`.
- Descriptions passed to `check` must not include addresses or timings, because Task 4 compares them across builds.

- [ ] **Step 1: Write the failing test registration**

Register `paoo demo` as a test named `demo` in both build systems.

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL because the `demo` subcommand doesn't exist.

- [ ] **Step 3: Implement `runLab3()` following the Lucrarea 3 table in spec section 9**

Required checks:
- Rejected `at`, `set` and constructor calls throw, and the image is unchanged.
- `FilterPreset` keeps its name and radius.
- `ShallowImage` copies share an address, and a write through one is visible through the other.
- `Image` copies have different addresses and stay independent.
- A move keeps the buffer address and empties the source.

Neutralize the `ShallowImage` pointer before scope exit. Call `std::move` explicitly in the move demo, because C++17's guaranteed elision means initializing from a returned temporary never calls the move constructor. Print copy and move timings for a 4096×4096×3 image without checking them.

- [ ] **Step 4: Implement the `demo` command in `src/main.cpp`**

- [ ] **Step 5: Run to verify it passes in Release as well**

Run: `scripts/verify.sh cmake-debug && scripts/verify.sh cmake-release && scripts/verify.sh meson-debug && scripts/verify.sh sanitize`
Expected: all pass, and `build/cmake-release/src/paoo demo` prints the same `[PASS]` lines as Debug. This confirms the `assert` bug from spec section 4 is gone.

- [ ] **Step 6: Commit**

Message: `Add Lucrarea 3 demo and demo command`.

---

### Task 4: Quality tooling, and the Lucrarea 3 checkpoint

**Files:**
- Create: `.clang-format`, `.clang-tidy`, `lefthook.yml`, `.github/workflows/ci.yml`
- Modify: `scripts/verify.sh`, source files only as formatting or lint requires

**Interfaces:**
- Produces `verify.sh` steps `format`, `lint`, `leaks` and `parity`. The full order is `format`, `lint`, `cmake-debug`, `cmake-release`, `meson-debug`, `meson-release`, `parity`, `sanitize`, `leaks`.

- [ ] **Step 1: Prove `leaks` catches a leak**

Build a throwaway program in the session scratchpad that leaks a `new int[4]`.
Run: `leaks --atExit -- <program>`
Expected: non-zero exit reporting 1 leak. This confirms a failed check will fail the step. Delete the program.

- [ ] **Step 2: Add the `verify.sh` steps**

Details not in spec section 8:
- Prepend `"$(brew --prefix llvm)/bin"` to `PATH` when it exists, because `clang-format` and `clang-tidy` aren't on `PATH`.
- `lint` first configures `build/cmake-debug`, because it runs before the build steps and needs the compile database. It then uses `-p build/cmake-debug` and passes `--extra-arg=-isysroot$(xcrun --show-sdk-path)`, because Homebrew's `clang-tidy` doesn't find the Apple SDK by itself. It must not scan `third_party/` or `build/`.
- `leaks` runs `leaks --atExit -- build/cmake-debug/src/paoo demo`. Task 9 adds the `filter` run.
- `parity` runs `paoo demo` from the CMake and Meson Debug and Release builds, and fails unless the sorted `[PASS]` lines from all four match. The Item 4 order value and timings don't use `check`, so they don't affect this.

- [ ] **Step 3: Write `.clang-format` and `.clang-tidy`**

`.clang-format`: `BasedOnStyle: LLVM`, `IndentWidth: 4`, `ColumnLimit: 100`, `AccessModifierOffset: -4`. `.clang-tidy`: `Checks: 'bugprone-*,clang-analyzer-*,performance-*'` with `HeaderFilterRegex` limited to `inc/`. Silence `ShallowImage`'s ownership findings with a targeted `// NOLINT(<check>)` and a reason.

- [ ] **Step 4: Write `lefthook.yml`**

`pre-commit`: runs `clang-format -i` on staged `*.{cpp,hpp}`, excluding `third_party/`, with `stage_fixed: true`. Use the same LLVM `PATH` prefix. `pre-push`: runs `scripts/verify.sh format`, `lint`, `cmake-debug` and `meson-debug`.

- [ ] **Step 5: Write `.github/workflows/ci.yml`**

Runs on `push` and `pull_request` on `macos-latest`. Each job checks out the repo, runs `brew install llvm meson ninja`, then calls `scripts/verify.sh`:
- `format-lint`: `format`, then `cmake-debug` (for the compile database), then `lint`.
- `cmake`: `cmake-debug`, `cmake-release`.
- `meson`: `meson-debug`, `meson-release`.
- `sanitize`: `sanitize`.
- `leaks`: `cmake-debug`, `leaks`.

Run: `ruby -e 'require "yaml"; YAML.load_file(".github/workflows/ci.yml"); YAML.load_file("lefthook.yml"); puts "ok"'`
Expected: `ok`.

- [ ] **Step 6: Install and run the hooks**

Run: `lefthook install && lefthook run pre-commit --all-files && lefthook run pre-push`
Expected: all commands succeed. Fix any formatting or lint findings.

- [ ] **Step 7: Run the full checkpoint**

Run: `scripts/verify.sh`
Expected: exit 0 with every step passing.

- [ ] **Step 8: Commit**

Message: `Add formatting, linting, leak checks, lefthook and CI`.

---

### Task 5: Lucrarea 4 demos

**Files:**
- Create: `src/demos/lab4.cpp`, `src/demos/lab4_order_a.cpp`, `src/demos/lab4_order_b.cpp`
- Modify: `inc/Demos.hpp`, `inc/DemoTypes.hpp`, `src/main.cpp`, `src/CMakeLists.txt`, `src/meson.build`

**Interfaces:**
- Produces in `Demos.hpp`: `void runLab4();`
- Produces in `DemoTypes.hpp`: `struct Probe`, with static counters `defaultConstructions`, `copyConstructions`, `copyAssignments` and `static void reset()`.
- Order demo:
    - `lab4_order_b.cpp`: `int orderDynamicValue();`, a global `int orderGlobalValue = orderDynamicValue();`, and `int& orderLocalValue();`, a function-local static.
    - `lab4_order_a.cpp`: a global `int orderCopiedAtStartup = orderGlobalValue;` and `int orderLocalAtStartup = orderLocalValue();`.
    - List `a` before `b` in both builds, and compile both into `paoo`, not `image_lib` (spec section 6).

- [ ] **Step 1: Extend the test registration**

Change the `paoo demo` range to allow `--lab 4`, and add `static_assert`s at the top of `lab4.cpp` so a wrong trait fails compilation:

```cpp
static_assert(!std::is_copy_assignable<FilterPreset>::value, "const members block operator=");
static_assert(!std::is_copy_constructible<Logger>::value, "Logger is deleted");
static_assert(!std::is_copy_constructible<UncopyableExample>::value, "private base blocks copy");
static_assert(!std::is_default_constructible<HasOwnConstructor>::value, "own ctor removes default");
static_assert(std::is_copy_assignable<PlainStruct>::value, "compiler generates operator=");
```

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL to compile, because the demo types don't exist.

- [ ] **Step 3: Implement Items 4, 5 and 6 following spec section 9**

Required checks:
- **Item 4:**
    - A new `Image` is zero-filled.
    - The assign-in-body class records 1 default construction and 1 assignment, and no copy construction.
    - The initializer-list class records 1 copy construction, and no default construction or assignment.
    - `orderLocalAtStartup` equals the expected value.
- **Item 5:**
    - Copying a struct with a `Probe` member increments `copyConstructions`.
    - Assigning it increments `copyAssignments`.
    - `ShallowImage` shares an address after a copy.
- **Item 6:** two `Logger::instance()` calls return one address.

Print `orderCopiedAtStartup` without checking it. Never read an indeterminate value.

- [ ] **Step 4: Run the Lucrarea 4 checkpoint**

Run: `scripts/verify.sh`
Expected: exit 0, and `paoo demo --lab 4` prints each item's output.

- [ ] **Step 5: Commit**

Message: `Add Lucrarea 4 demos for Items 4, 5 and 6`.

---

### Task 6: `Image::operator=` and Items 10 and 11

**Files:**
- Create: `src/demos/lab5.cpp`
- Modify: `inc/Image.hpp`, `src/Image.cpp`, `tests/test_image.cpp`, `inc/Demos.hpp`, `src/main.cpp`, `src/CMakeLists.txt`, `src/meson.build`

**Interfaces:**
- Produces: `Image& Image::operator=(const Image& other);` and `void runLab5();`

- [ ] **Step 1: Write the failing tests**

```cpp
static_assert(std::is_same<decltype(std::declval<Image&>() = std::declval<const Image&>()),
                           Image&>::value, "");
TEST_CASE("assignment deep-copies across sizes") {
    // Target 1x1x1, source 3x2x4 with known pixels: the target gets the dimensions and pixels.
    // Addresses differ, and changing the source leaves the target unchanged.
}
TEST_CASE("assignment chains") {
    // a = b = c leaves a and b with c's pixels, and in separate buffers.
}
TEST_CASE("self-assignment keeps pixels and address") {
    // Through a reference alias: pixels are unchanged, and getData() has the same address.
}
TEST_CASE("assigning from empty empties the target") {
    // getData() == nullptr.
}
TEST_CASE("a moved-from image can be assigned to again") {
    // After a move, assign a valid image: the moved-from object holds its pixels.
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL because copy assignment is deleted.

- [ ] **Step 3: Implement `operator=`**

Use the order from spec section 7: identity test, `cloneBuffer`, free the old buffer, commit the fields, return `*this`. Do not implement it through the copy constructor.

- [ ] **Step 4: Run to verify it passes**

Run: `scripts/verify.sh cmake-debug`
Expected: all tests pass.

- [ ] **Step 5: Implement Items 10 and 11 in `src/demos/lab5.cpp`**

Follow spec section 9. `VoidAssignImage` uses a `static_assert` that its `operator=` returns `void`. `NaiveImage` frees and zeroes its dimensions before copying, and must not read freed memory. Mark its `bugprone-unhandled-self-assignment` finding with a `// NOLINT` and a reason.

Required checks:
- `a = b = c` makes all three equal.
- `NaiveImage` is empty after `images[i] = images[j]` with `i == j`.
- `Image` keeps its pixels in the same scenario.

- [ ] **Step 6: Run under sanitizers**

Run: `scripts/verify.sh sanitize`
Expected: pass, which proves `NaiveImage` is free of undefined behaviour.

- [ ] **Step 7: Commit**

Message: `Add Image assignment and Items 10 and 11 demos`.

---

### Task 7: `LabeledImage`, Item 12, and the Lucrarea 5 checkpoint

**Files:**
- Create: `inc/LabeledImage.hpp`, `src/LabeledImage.cpp`, `tests/test_labeled_image.cpp`
- Modify: `src/demos/lab5.cpp`, `src/CMakeLists.txt`, `src/meson.build`, `tests/CMakeLists.txt`, `tests/meson.build`

**Interfaces:**
- Consumes: `Image` copy constructor and `operator=`.
- Produces:

```cpp
class LabeledImage : public Image {
public:
    LabeledImage();
    LabeledImage(const Image& image, const std::string& label);
    LabeledImage(const LabeledImage& other);
    LabeledImage& operator=(const LabeledImage& other);
    const std::string& getLabel() const;
private:
    std::string label;
};
```

- [ ] **Step 1: Write the failing tests**

```cpp
TEST_CASE("copy keeps pixels and label") {
    // The copy has equal pixels, a different address, and the same label.
}
TEST_CASE("assignment replaces pixels and label") {
    // Differently sized target and source: the target gets the source's dimensions, pixels and label.
}
TEST_CASE("self-assignment keeps pixels and label") {
    // Through a reference alias.
}
TEST_CASE("default LabeledImage is empty with an empty label") {
    // isEmpty() is true and getLabel() is "".
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL because `LabeledImage.hpp` doesn't exist.

- [ ] **Step 3: Implement `LabeledImage`**

Initialize `Image(other)` in the copy constructor's initializer list, and call `Image::operator=(other)` in assignment. Add a comment that `~Image` isn't virtual, so it must never be deleted through `Image*` (spec section 7).

- [ ] **Step 4: Run to verify it passes**

Run: `scripts/verify.sh cmake-debug`
Expected: all tests pass.

- [ ] **Step 5: Add Item 12 to `runLab5()` following spec section 9**

Required checks:
- A type with a newly added field loses it in its hand-written copy.
- `BrokenLabeledImage` has empty pixels after copy construction, and keeps its old pixels after assignment.
- `LabeledImage` is correct in both scenarios.

If a compiler warns about the missing base initializer in `BrokenLabeledImage`, silence it locally with a comment explaining why.

- [ ] **Step 6: Run the Lucrarea 5 checkpoint**

Run: `scripts/verify.sh`
Expected: exit 0, and `paoo demo` runs all three labs.

- [ ] **Step 7: Commit**

Message: `Add LabeledImage and Item 12 demo`.

---

### Task 8: Filters

**Files:**
- Create: `inc/Filters.hpp`, `src/Filters.cpp`, `tests/test_filters.cpp`
- Modify: `src/CMakeLists.txt`, `src/meson.build`, `tests/CMakeLists.txt`, `tests/meson.build`

**Interfaces:**
- Produces:

```cpp
Image grayscale(const Image& image);
Image invert(const Image& image);
Image boxBlur(const Image& image, int radius);       // radius 1..64, else std::invalid_argument
Image applyFilterSpec(const Image& image, const std::string& spec); // std::invalid_argument on bad spec
```

- [ ] **Step 1: Write the failing tests**

```cpp
TEST_CASE("grayscale uses rounded luma and keeps alpha") {
    // 1x1 RGBA (255,0,0,77) gives (76,76,76,77), since 0.299*255 = 76.245.
    // 1x1 RGB (0,255,0) gives (150,150,150), since 0.587*255 = 149.685.
}
TEST_CASE("grayscale copies 1- and 2-channel input") {
    // Pixels are unchanged for a 1-channel and a 2-channel image.
}
TEST_CASE("invert keeps alpha") {
    // RGB (0,10,255) gives (255,245,0). RGBA (0,10,255,77) gives (255,245,0,77).
    // 2-channel (10,77) gives (245,77). 1-channel (10) gives (245).
}
TEST_CASE("box blur averages with clamped edges") {
    // 3x1x1 pixels (0,0,90) at radius 1:
    // x=0 averages (0,0,0)=0, x=1 averages (0,0,90)=30, x=2 averages (0,90,90)=60.
}
TEST_CASE("blur on 1x1 returns the same pixel") {
    // Radius 64 on a single pixel (42) gives 42.
}
TEST_CASE("filters leave the input unchanged and return its dimensions") {}
TEST_CASE("specs select filters") {
    // "grayscale", "invert" and "blur:2" match the direct function calls.
}
TEST_CASE("bad specs are rejected") {
    // "", "sharpen", "blur", "blur:", "blur:abc", "blur:2x", "blur:0", "blur:65", "grayscale:3",
    // "invert:1" each throw std::invalid_argument.
}
```

The blur's clamping weights a repeated edge pixel by each time it appears in the window, as the expected values above show. Rounded averages use integer rounding to nearest.

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL because `Filters.hpp` doesn't exist.

- [ ] **Step 3: Implement the filters in `src/Filters.cpp`**

Use plain loops and no threads. Parse the radius with `std::stoi` and its `pos` argument so a value like `2x` is rejected, and convert its exceptions to `std::invalid_argument`.

- [ ] **Step 4: Run to verify it passes**

Run: `scripts/verify.sh cmake-debug && scripts/verify.sh meson-debug`
Expected: all tests pass.

- [ ] **Step 5: Commit**

Message: `Add grayscale, invert and box blur filters`.

---

### Task 9: `ImageIO` and the `filter` command

**Files:**
- Create: `inc/ImageIO.hpp`, `src/ImageIO.cpp`, `tests/test_image_io.cpp`, `tests/test_cli.sh`, `tests/data/sample.png`
- Modify: `src/main.cpp`, `scripts/verify.sh`, `src/CMakeLists.txt`, `src/meson.build`, `tests/CMakeLists.txt`, `tests/meson.build`

**Interfaces:**
- Consumes: `applyFilterSpec`, `LabeledImage`, `Image::operator=`.
- Produces: `LabeledImage loadImage(const std::string& path);` and `void saveImage(const Image& image, const std::string& path);`
- `tests/test_cli.sh <paoo path> <sample.png> <output dir>` is registered as test `filter_cli` in both builds.

- [ ] **Step 1: Write the failing tests**

```cpp
TEST_CASE("PNG round trip is lossless") {
    // 3x2x4 known pixels: saved and loaded data are equal. The label is the path.
}
TEST_CASE("uppercase extensions work") {
    // saveImage to "OUT.PNG" succeeds and loadImage reads it.
}
TEST_CASE("RGBA saved as JPEG loads with 3 channels") {
    // 4x4x4 saved to ".jpg": loadImage returns 3 channels and 4x4 dimensions.
}
TEST_CASE("jpeg and bmp extensions save") {
    // ".jpeg" and ".bmp" both produce files that load.
}
TEST_CASE("errors throw runtime_error") {
    // A missing input file, a text file named ".png", an unsupported ".gif" output,
    // no extension, a missing output directory, and saving an empty Image all throw.
}
```

`tests/test_cli.sh` runs with `set -u` and fails with a clear message on any mismatch:
- `filter -i sample -o out.png -f grayscale -f blur:2` exits 0, creates a file and prints a line containing `Wrote`.
- `paoo` with no arguments exits non-zero and prints usage.
- Running with no `-f`, a missing input, `-f blur:65`, `-f sharpen` and output `out.gif` each exits non-zero and prints to `stderr`.
- An existing output file keeps its bytes after a failing `-f blur:65` run.
- Using the same path for `-i` and `-o` on a copy of the sample exits 0, and the file still loads through a later `filter` run.

- [ ] **Step 2: Run to verify it fails**

Run: `scripts/verify.sh cmake-debug`
Expected: FAIL because `ImageIO.hpp` doesn't exist.

- [ ] **Step 3: Implement `ImageIO`**

Hold the `stbi_load` result in a `std::unique_ptr<unsigned char, decltype(&stbi_image_free)>` while copying. Lower-case the extension before matching `.png`, `.jpg`, `.jpeg` or `.bmp`. JPEG quality is 90. Treat a 0 return from any `stbi_write_*` call as failure.

A 4-channel image saved as JPEG must be valid. The test above confirms how stb handles it, and if it doesn't produce a 3-channel file, drop the alpha channel first.

- [ ] **Step 4: Implement `filter` in `src/main.cpp`**

Flow: `loadImage`, assign to `Image current`, then `current = applyFilterSpec(current, spec)` for each `-f`, then `saveImage`, then print `Wrote <output> from <label>`. `-i`, `-o` and at least one `-f` are required. Tracing stays off.

- [ ] **Step 5: Create the fixture, and extend the `leaks` step**

Create `tests/data/sample.png` as a small RGBA PNG, 8×8 or smaller. Add to `verify.sh leaks`: `leaks --atExit -- build/cmake-debug/src/paoo filter -i tests/data/sample.png -o build/leaks-out.png -f grayscale -f blur:2`.

- [ ] **Step 6: Run the full verification**

Run: `scripts/verify.sh`
Expected: exit 0, including `filter_cli` under both builds and sanitizers, and no leaks.

- [ ] **Step 7: Commit**

Message: `Add ImageIO and filter command`.

---

### Task 10: Documentation and final check

**Files:**
- Modify: `README.md`, `docs/cmake-vs-meson.md`, `docs/2026-10-18-sensor-logger-design.md`, `docs/2026-10-18-sensor-logger-plan.md`

- [ ] **Step 1: Update the documents**

- `README.md`: what the tool does, prerequisites (`cmake`, `meson`, `ninja`, Homebrew `llvm`, `lefthook`), build and test commands for both systems, `lefthook install`, usage for `demo` and `filter`, and `scripts/verify.sh` with its steps.
- `docs/cmake-vs-meson.md`: replace the old snippets and `Lab_03` names with the current build, including how each system gets its libraries.
- Both sensor documents: add one line at the top saying they are superseded by the image filter design.

- [ ] **Step 2: Confirm no sensor code remains**

Run: `git grep -n SensorLogger -- inc src tests CMakeLists.txt meson.build`
Expected: no output, and exit code 1.

- [ ] **Step 3: Run the final verification from clean**

Run: `rm -rf build && scripts/verify.sh`
Expected: exit 0 across every step.

- [ ] **Step 4: Check each graded item by running it**

Run: `build/cmake-debug/src/paoo demo --lab 3`, then `--lab 4`, then `--lab 5`.
Expected: each item in `docs/task.md` for these labs appears in the output with passing checks.

- [ ] **Step 5: Commit**

Message: `Document the image filter tool and retire sensor docs`.

- [ ] **Step 6: Report, and ask before pushing**

Tell Paul the branch is ready. CI can't be confirmed until the branch is pushed, so ask whether to push it. If the hosted `leaks` job fails for reasons unrelated to the code, apply the rule in spec section 8 and tell Paul.
