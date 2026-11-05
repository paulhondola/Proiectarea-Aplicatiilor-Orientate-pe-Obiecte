# Image Filter Pipeline: Design

**Date:** 2026-11-05
**Replaces:** the `SensorLogger` topic (`docs/2026-10-18-sensor-logger-design.md`)
**Delivers now:** the Lucrarea 3 material on the new topic, plus Lucrarea 4 and Lucrarea 5
**Plans for, but does not build:** Lucrarea 6, 7 and 8

---

## 1. Intent

The course requires one C++ application to demonstrate a growing list of concepts, from object lifetime up to threads and semaphores (`docs/task.md`). `SensorLogger` satisfies Lucrarea 3, but its topic gives the later labs little real work to do.

This project replaces it with a command-line image filter tool. Images give ownership real weight: copying one is visibly expensive, images change size on assignment, and later labs get natural uses for interfaces (filters), RAII (image library buffers) and threads (stages passing images along).

The program has two jobs:

1. **Be a working tool:** read an image, apply filters, write the result.
2. **Teach each graded concept:** a `demo` command shows why each concept is needed, usually by showing what goes wrong without it, as the task requires.

## 2. Goals, non-goals, anti-goals

### Goals

- One application that grows lab by lab, not one folder per lab.
- Every graded item in Lucrarea 3, 4 and 5 has a clear demo that prints what happens and checks the result.
- CMake and Meson both build and test the project, with neither treated as secondary.
- External libraries handle what the labs don't grade: file formats, arguments and tests.

### Non-goals

- Building Lucrarea 6, 7 or 8 now. Section 10 only records how the design must leave room for them.
- Performance work. Filters use plain loops. The `gemm-bench` micro-kernel and cache tiling are out of scope and can be revisited after the labs are finished.
- Deployment. CI checks the project, but there is nothing to deploy. Add a release step if a downloadable binary is ever wanted.
- A large image library such as OpenCV.

### Anti-goals

These would make the work a failure even if it ran:

- A library hides a graded concept. `Image`, its copying and, later, the threading must stay hand-written.
- A demo relies on undefined behaviour to make its point. It must stay clean under ASan and UBSan.
- Demos or checks behave differently in Release. The current code has this bug (section 4).
- A build system silently falls behind the other one.

## 3. Decisions

### Made by Paul

- Topic: image filter pipeline.
- One growing application.
- External libraries are welcome, such as argument parsers and image format libraries.
- Meson is developed in parallel with CMake, and both must work.
- The repo-root layout is acceptable: the course doesn't require a `Lab/Paul/` path or a folder per lab.
- Paul builds and shows the labs on his own Mac, so builds only need to be proven there. Downloading CLI11 and Catch2 on the first build is acceptable, and no Linux or Windows support is required.
- Memory leaks on macOS are checked with `leaks --atExit --`.
- The project has a lefthook file and CI covering formatting, linting, tests, ASan and UBSan.

### Made by the design author (open to change)

- The specific libraries and versions in section 8.
- The `demo` and `filter` subcommands.
- `LabeledImage` as the Lucrarea 5 derived class.
- `Logger` as the Item 4 and Item 6 example.
- Removing `SensorLogger` from the tree, with a `lab3-sensor-logger` git tag kept locally on the last commit that contains it.
- Staying on C++14.
- `scripts/verify.sh` passes at the end of each lab's commits, so Lucrarea 3, 4 and 5 each leave a working checkpoint.
- Which checks run in which hook, and CI running on macOS only (section 8).

## 4. Problems in the current code that must not carry over

| Problem | Where | Requirement in the new code |
|---|---|---|
| Side effects inside `assert` vanish in Release. The demo then prints `Readings (0/3)` and still exits 0. | `src/main.cpp`, for example line 58 | Demo checks use a `check()` helper that always runs and exits non-zero on failure. |
| The copy constructor changes the data (`location + " [Copy]"`). | `src/SensorLogger.cpp:34` | Copies are faithful. Logs identify objects by buffer address. |
| A `const` member makes faithful assignment impossible. | `inc/SensorLogger.hpp:8` | `Image` has no `const` members. A small demo type shows the `const` behaviour instead. |
| No default constructor, so the Item 12 forgotten-base bug can't be silent. | `SensorLogger` | `Image()` creates an empty image. |
| CMake has no warning flags or tests, unlike Meson. | `CMakeLists.txt` | Both builds use the same warnings and run the same tests. |

## 5. Application behaviour

The executable stays named `PAOO`. All visible text is in English, matching the existing code.

```text
PAOO demo [--lab 3|4|5]
PAOO filter -i <input> -o <output> -f <filter> [-f <filter> ...]
```

Running with no subcommand prints help and exits non-zero.

### `demo`

- Runs the chosen lab, or all labs in order when `--lab` is omitted.
- Turns on lifecycle tracing, so constructors, destructors and assignments log with buffer addresses.
- Exits 0 only if every check passed.

### `filter`

- Filters run in the order given. At least one is required.
- Tracing stays off, so normal output is one result line, such as `Wrote out.png from in.png`.
- Available filters:

| Name | Effect |
|---|---|
| `grayscale` | Sets colour channels to luma using weights 0.299, 0.587 and 0.114, rounded. The channel count doesn't change, alpha is kept, and 1- or 2-channel input is copied unchanged. |
| `invert` | Replaces each colour channel `v` with `255 - v`. Alpha is kept. |
| `blur:<radius>` | Box blur over all channels with clamped edges. Radius is an integer from 1 to 64. |

- **Input:** any format `stb_image` reads, loaded as 8-bit interleaved with its own channel count.
- **Output:** chosen by extension: `.png`, `.jpg`, `.jpeg` or `.bmp`. JPEG quality is 90.
- **Errors:** an unknown filter, bad radius, unreadable input, unsupported extension or failed write prints one message to `stderr` and exits non-zero.
- The output file is only written after every filter has succeeded.

## 6. Code structure

The existing conventions stay: `inc/` for headers, `src/` for sources, `.hpp`, include guards, four-space indentation, camelCase methods and `getX()` accessors.

```text
CMakeLists.txt                 meson.build
src/CMakeLists.txt             src/meson.build
tests/CMakeLists.txt           tests/meson.build
third_party/CMakeLists.txt     third_party/meson.build
subprojects/cli11.wrap         subprojects/catch2.wrap
scripts/verify.sh
lefthook.yml                   .github/workflows/ci.yml
.clang-format                  .clang-tidy
inc/
    Image.hpp  LabeledImage.hpp  Filters.hpp  ImageIO.hpp  Logger.hpp
    Demos.hpp                  (runLab3/4/5 and check())
    DemoTypes.hpp              (types shared between demos)
src/
    Image.cpp  LabeledImage.cpp  Filters.cpp  ImageIO.cpp  Logger.cpp
    stb_impl.cpp               (the one file that compiles stb)
    main.cpp                   (CLI11 setup and dispatch only)
    demos/
        lab3.cpp  lab4.cpp  lab5.cpp
        lab4_order_a.cpp  lab4_order_b.cpp
tests/
    *.cpp
    data/sample.png            (very small committed fixture)
third_party/stb/
    stb_image.h  stb_image_write.h  LICENSE
```

**Targets, the same in both builds:**

- `image_lib`: static library with `Image`, `LabeledImage`, `Filters`, `ImageIO` and `Logger`.
- `PAOO`: executable with `main.cpp` and every file in `src/demos/`.
- `paoo_tests`: Catch2 test executable.

**Demo files must be compiled directly into `PAOO`, not into `image_lib`.** A linker can drop an unreferenced object file from a static library, along with its global constructors, and the Item 4 demo depends on those constructors.

Demo-only types stay inside their demo file unless two labs share them, in which case they go in `DemoTypes.hpp`.

## 7. Core classes

### `Image`

Owns one heap buffer of 8-bit interleaved pixels through a raw `unsigned char*`. The raw pointer is deliberate: Lucrarea 3 grades writing the destructor and copy and move operations by hand.

```cpp
class Image {
public:
    Image();                                    // empty: 0x0x0, nullptr
    Image(int width, int height, int channels); // zero-filled
    ~Image();
    Image(const Image& other);                  // deep copy
    Image(Image&& other) noexcept;              // takes the buffer; other becomes empty
    Image& operator=(const Image& other);       // added for Lucrarea 5

    int getWidth() const;
    int getHeight() const;
    int getChannels() const;
    bool isEmpty() const;
    std::size_t getByteCount() const;

    unsigned char at(int x, int y, int channel) const;      // bounds-checked
    void set(int x, int y, int channel, unsigned char value);
    const unsigned char* getData() const;
    unsigned char* getData();

private:
    static unsigned char* cloneBuffer(const Image& source); // shared by copy paths (Item 12)
    int width;
    int height;
    int channels;
    unsigned char* pixels;
};
```

**Rules:**

- Width and height must be positive, and channels must be 1 to 4. Otherwise the constructor throws `std::invalid_argument`. It also rejects sizes whose byte count would overflow `std::size_t`.
- `at` and `set` throw `std::out_of_range`, which is the encapsulation demo for Lucrarea 3.
- `operator=` does an identity test, allocates and copies through `cloneBuffer`, then frees the old buffer and commits. A failed allocation leaves the target unchanged.
- There is no move assignment yet, so `a = std::move(b)` deep-copies safely. Lucrarea 7 adds it.
- Every special member logs through `Logger`.

### `LabeledImage : public Image`

An image plus `std::string label`, holding the source path for loaded images. It writes its copy constructor and `operator=` by hand and explicitly copies the base part in each.

`~Image` is not virtual, so a `LabeledImage` must never be deleted through an `Image*`. Lucrarea 8 must therefore use `std::unique_ptr<LabeledImage>` in its queue. Lucrarea 7 applies polymorphism to `Filter`, not to `Image`.

`filter` uses it to report the source path. Assigning a filter result to `Image current` intentionally keeps only the pixels.

### `Logger`

```cpp
class Logger {
public:
    static Logger& instance();                 // function-local static (Item 4)
    Logger(const Logger&) = delete;            // only one exists (Item 6)
    Logger& operator=(const Logger&) = delete;
    void setEnabled(bool enabled);
    void trace(const std::string& message) const;
private:
    Logger();
    bool enabled;
};
```

### `Filters`

Free functions, each taking `const Image&` and returning a new `Image`: `grayscale`, `invert` and `boxBlur`. Lucrarea 7 wraps them behind an interface.

### `ImageIO`

- `LabeledImage loadImage(const std::string& path)`
- `void saveImage(const Image& image, const std::string& path)`

Both throw `std::runtime_error`, using `stbi_failure_reason()` for load failures.

`stbi_load` returns a `malloc`-based buffer. `ImageIO.cpp` holds it in a `std::unique_ptr` with `stbi_image_free` as the deleter while copying it into an `Image`, so an exception can't leak it. Lucrarea 6 replaces that with its own RAII class.

## 8. Build systems and libraries

| Library | Version | CMake | Meson |
|---|---|---|---|
| CLI11 | v2.7.2 | `FetchContent`, `CLI11::CLI11` | wrap `2.7.2-1`, `dependency('CLI11')` |
| Catch2 | v3.16.1 | `FetchContent`, `Catch2::Catch2WithMain` | wrap `3.16.1-1`, `dependency('catch2-with-main')` |
| stb | commit `d14d46f6b44456eb88beac58cba833a4135d4c89` | vendored in `third_party/stb` | same vendored files |

stb is vendored because WrapDB doesn't provide it. Both build systems then use the same files, with no download. Its MIT or public-domain license file is kept beside it. `stb_impl.cpp` is the only file that defines the `*_IMPLEMENTATION` macros, and stb is included as a system header so its warnings don't appear.

**Both builds must:**

- Use C++14 with compiler extensions off.
- Enable `-Wall -Wextra -Wpedantic` or equivalent for project code only, without `-Werror`. If a deliberately broken demo type triggers a warning, silence it locally with a comment explaining why.
- Provide a sanitizer build: `-DPAOO_SANITIZE=ON` in CMake and `-Db_sanitize=address,undefined` in Meson.
- List the same source files in the same order. The Item 4 demo prints the initialization order it observes.
- Register the same tests: `paoo_tests`, `PAOO demo`, and a `PAOO filter` run on `tests/data/sample.png`. Test output files go in the build directory.

**CMake details:** raise the minimum to 3.14 for `FetchContent_MakeAvailable`, and split the build across the `CMakeLists.txt` files shown in section 6, because the Lucrarea 7 rubric refers to `CMakeLists.txt` files in the plural.

**Meson details:** mirror the structure with `subdir()`. Commit the `.wrap` files, and ignore downloaded subproject folders and the Meson build directories in `.gitignore`. Raise `meson_version` if the wraps need it.

**Networking:** the first build of either system needs internet access for CLI11 and Catch2.

**`scripts/verify.sh`** is the one definition of the checks. With no arguments it runs every step in order and stops at the first failure. With a step name it runs only that step, so lefthook and CI call the same commands instead of keeping their own copies.

| Step | What it does |
|---|---|
| `format` | `clang-format --dry-run --Werror` on project files, excluding `third_party/` |
| `lint` | `clang-tidy` on project sources, using the compile database from CMake (`CMAKE_EXPORT_COMPILE_COMMANDS`) |
| `cmake-debug`, `cmake-release` | Build and `ctest` |
| `meson-debug`, `meson-release` | Build and `meson test` |
| `sanitize` | ASan and UBSan builds under both systems, then their tests |
| `leaks` | `leaks --atExit -- <PAOO> demo` and the `filter` smoke run, on a Debug build without sanitizers, because `leaks` can't inspect ASan-instrumented programs. Any leak fails the step. |

**Tool configuration:**

- `.clang-format` matches the existing style: four-space indentation, with no reformatting of `third_party/`.
- `.clang-tidy` enables `bugprone-*`, `clang-analyzer-*` and `performance-*`. Checks that object to the required raw-pointer ownership stay off.
- Deliberately broken demo types use a targeted `// NOLINT(<check>)` with a reason. For example, `NaiveImage` is expected to trigger `bugprone-unhandled-self-assignment`.

**`lefthook.yml`:**

- `pre-commit` runs `clang-format` on staged C++ files and re-stages the result, because this hook should be fast.
- `pre-push` runs the `format`, `lint`, `cmake-debug` and `meson-debug` steps.
- Sanitizers, Release builds and `leaks` run through CI or a full `verify.sh`, to keep pushes quick.

**`.github/workflows/ci.yml`** runs on pushes and pull requests:

- Runs on `macos-latest`, matching the machine used to show the labs, with one job for each group: `format` and `lint`, CMake, Meson, `sanitize`, and `leaks`.
- Installs tools through Homebrew: `llvm` for `clang-format` and `clang-tidy`, plus `meson` and `ninja`.
- Caches nothing at first. Add caching only if run times become a problem.
- `leaks` may behave differently on hosted runners. If it can't run there for reasons unrelated to the code, that job may be removed from CI, as long as the step keeps passing locally. Record the reason in the workflow file.

## 9. Demos

Each demo prints a short explanation, runs the example and verifies it with `check()`. Compile-time facts use `static_assert`. A demo that needs a bad example must keep it free of undefined behaviour and of leaks, because the sanitizer and `leaks` steps run every demo.

### Lucrarea 3 (ported to `Image`)

| Requirement | Points | Demo |
|---|---|---|
| CMake build | 3p | Section 8 |
| Encapsulation | 1p | Pixels are private. Out-of-range `at` and `set` and invalid sizes are rejected, and the image stays intact. |
| Constructor initialization | 1p | `Image`'s initializer list, where `pixels` depends on the dimensions declared before it. `FilterPreset`, with `const` members, can only be initialized there. |
| Destructor frees the heap | 1p | A scoped image logs its allocation and release. |
| Copy constructor | 2p | `ShallowImage` uses the generated copy: both objects share one address, and a write through one changes the other. Its pointer is neutralized before scope exit, and the output explains the double free this avoids. Then `Image` shows independent buffers. |
| Move constructor | 2p | Moving a 4096×4096 RGB image keeps the same buffer address and empties the source. Print copy and move times side by side, without checking exact timings. |

### Lucrarea 4

**Item 4: initialize objects before use**

- **Built-in types:** `Image(w, h, c)` guarantees zeroed pixels, and the demo checks it. The text explains that `new unsigned char[n]` alone leaves values indeterminate. The demo never reads such values.
- **Initialization versus assignment:** a `Probe` member type counts its operations. A class that assigns in its constructor body costs a default construction plus an assignment. One using an initializer list costs one copy construction.
- **Declaration order:** explain that members initialize in declaration order, regardless of list order. `Image` depends on this.
- **Order across source files:**
    - `lab4_order_b.cpp` holds a global `int` set through a function call.
    - A global in `lab4_order_a.cpp` copies it at startup. Integers are zero-initialized first, so a `0` is well defined, and the standard doesn't guarantee which order runs.
    - Print the value observed, without checking it.
    - A function-local static version must always return the right value, and that is checked.

**Item 5: what C++ silently writes and calls**

- `static_assert`s show what a plain struct receives, and that declaring any constructor removes the default one.
- A struct with a `Probe` member shows generated copy functions calling each member's own copy function.
- `FilterPreset` shows the compiler refusing to generate `operator=` for `const` members: `static_assert(!std::is_copy_assignable<FilterPreset>::value)`.
- `ShallowImage` is the case where the generated copy compiles and is wrong.

**Item 6: explicitly disallow unwanted functions**

- `Logger` is the real example: `= delete`, because only one should exist. `static_assert` proves it.
- A small class inheriting a private `Uncopyable` base shows the book's C++03 form. The text explains the older private-and-undefined approach and why its error appears later, at link time.

### Lucrarea 5

**Item 10: return a reference to `*this`**

- `a = b = c` works, and all three images end up equal.
- `static_assert` confirms `operator=` returns `Image&`.
- A demo type whose `operator=` returns `void` can't chain. Show this with a type trait, so the program still compiles.

**Item 11: handle assignment to self**

- Self-assignment often comes through aliases, so the demo uses `images[i] = images[j]` with `i == j`.
- `NaiveImage`'s `operator=` frees its buffer and resets its dimensions to zero, then copies from the source. Assigning to itself leaves it silently empty. This wipes the image without any undefined behaviour.
- The text explains that a version which keeps the dimensions would read freed memory instead.
- `Image` keeps its pixels in the same scenario.
- The text covers the identity test, careful statement order and copy-and-swap, and says which `Image` uses and why.

**Item 12: copy all parts of an object**

- **Forgotten member:** a demo type gains a new field that its hand-written copy functions don't handle, and the copy silently loses it.
- **Forgotten base part:** `BrokenLabeledImage` omits the base copy.
    - Its copy constructor silently produces empty pixels through `Image()`.
    - Its `operator=` silently keeps the old pixels.
    - `LabeledImage` gets both right.
- **Shared code:** `cloneBuffer` shows the correct way to share logic. The text explains why one copy function shouldn't call the other directly.

## 10. Room for later labs

These are not built now, but nothing here may block them.

| Lab | Planned addition | What this design must keep possible |
|---|---|---|
| **6** | `StbBuffer` RAII class in `ImageIO`, plus a choice of copying behaviour for Item 14. Filter coefficients shared by reference counting are one candidate. | `ImageIO` keeps stb calls private to one file. |
| **7** | `Filter` interface with at least two subclasses wrapping the free functions. `Image` move assignment and operators. A `Pipeline` class and an `imgproc` namespace. | Filters keep a uniform `Image -> Image` shape. |
| **8** | Stage threads passing `std::unique_ptr<LabeledImage>` through a bounded queue, and row threads inside filters. A hand-written semaphore using `std::mutex` and `std::condition_variable`, because `std::counting_semaphore` needs C++20. `std::lock_guard` in `Logger`. | Filters don't rely on global mutable state. `Logger` stays the only shared service. |

## 11. Definition of done

- `scripts/verify.sh` passes on the author's Mac, covering formatting, linting, all CMake and Meson configurations, sanitizer builds and `leaks`.
- `lefthook install` works and both hooks run. The CI workflow passes once the branch is pushed.
- `PAOO demo` prints the same checks and outcomes in Debug and Release. The Item 4 order value may differ between builds, and buffer addresses and timings will.
- `PAOO filter -i tests/data/sample.png -o out.png -f grayscale -f blur:2` produces a valid image, and each error case in section 5 exits non-zero.
- Catch2 tests cover:
    - `Image` construction, bounds, deep copy, move, chained assignment, self-assignment and assignment between different sizes.
    - `LabeledImage` copy and assignment, including the base part.
    - Each filter on a tiny image with known output.
    - A lossless PNG save and load, and load failure.
- Every Lucrarea 3, 4 and 5 item in sections 9 and `docs/task.md` has a demo a grader can find by running `PAOO demo --lab N`.
- No `SensorLogger` code remains. `README.md` explains build and usage for both systems. `docs/cmake-vs-meson.md` reflects the new build. Old sensor documents stay, with a one-line note saying they are superseded.
- Commits are organized by lab or concern, because the task grades organized commits.

Apple Clang has no LeakSanitizer, so the `leaks` step is what checks for memory leaks.

## 12. Not yet verified

- A probe building CLI11, Catch2 and stb together under both systems was blocked by the local permission policy, so it was not run. The versions and dependency names above come from published release and WrapDB metadata. **The first implementation task must prove this combination builds on C++14 under both systems before anything depends on it.** If a pinned version needs a newer standard, choose an older release of that library rather than raising the project standard.
## 13. Left to the builder

- Exact demo wording and output layout.
- How tests are split across files.
- How the `tests/data/sample.png` fixture is produced, provided it's small and committed.
- The internal shape of the `verify.sh` script and CLI11 setup.
- The exact `.clang-format` options and any further `clang-tidy` checks that prove too noisy to keep.
- The commit breakdown within the organized-by-lab rule.
