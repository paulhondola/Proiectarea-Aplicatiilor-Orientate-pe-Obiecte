# CMake vs. Meson Comparison

The project builds with both [CMake](../CMakeLists.txt) and [Meson](../meson.build). Both compile the same sources into the same targets, and run the same tests:

- `stb_impl`: a static library that compiles stb with its warnings silenced
- `image_lib`: a static library holding the application's classes
- `paoo`: the executable, with `main.cpp` and the lab demos
- `paoo_tests`: the Catch2 tests

Every new source or test must be added to both. `scripts/verify.sh` checks that both still work.

---

## 1. Side-by-side

### Choosing the C++ standard

```cmake
if(cxx_std_26 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 26)
else()
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

```meson
default_options: ['cpp_std=c++26,c++23', 'warning_level=3']
```

### Getting libraries

CMake downloads them when it configures, at the versions written in `CMakeLists.txt`:

```cmake
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.7.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cli11 catch2)
```

Meson downloads them using `.wrap` files committed in `subprojects/`, created with `meson wrap install`:

```meson
cli11_dep = dependency('CLI11', default_options: ['warning_level=0'])
```

WrapDB doesn't provide stb, so it's vendored in `third_party/stb`, and both systems use the same files.

### Building the application

```cmake
add_executable(paoo main.cpp demos/lab3.cpp ...)
target_link_libraries(paoo PRIVATE image_lib CLI11::CLI11 paoo_warnings)
```

```meson
paoo = executable('paoo', 'main.cpp', 'demos/lab3.cpp', ..., dependencies: [image_dep, cli11_dep])
```

---

## 2. Key differences

| Aspect | CMake | Meson |
| :--- | :--- | :--- |
| **Language** | A scripting language with global state and string expansion. | A declarative language that isn't Turing-complete, with typed objects. |
| **Standard fallback** | Written by hand, checking the compiler's feature list. | Built in, as a list of standards to try. |
| **Referencing targets** | By string name. A typo can be treated as a linker flag. | By variable. An undefined one is an immediate error. |
| **Sharing headers** | `target_include_directories(... PUBLIC ...)` passes them to whatever links the target. | `declare_dependency()` wraps a library with its headers. |
| **Warnings** | Added by hand through the `paoo_warnings` target, so they only apply to project code. | `warning_level=3` for the project, with subprojects set to 0. |
| **Sanitizers** | A project option, `PAOO_SANITIZE`, adds the flags before libraries are fetched. | Built-in `b_sanitize`, which also needs `b_lundef=false` under Clang. |
| **Build folders** | In-source builds are allowed unless blocked. | Always out of source. |
| **Default backend** | Makefiles on macOS, unless `-G Ninja` is used. | Ninja. |
| **Running tests** | Subproject tests are off by default here, so `ctest` runs only the project's. | `meson test` also runs subproject tests, so the project uses `--suite PAOO`. |

---

## 3. Commands

| Step | CMake | Meson |
| :--- | :--- | :--- |
| **Configure** | `cmake -S . -B build/cmake-debug -DCMAKE_BUILD_TYPE=Debug` | `meson setup build/meson-debug --buildtype=debug` |
| **Compile** | `cmake --build build/cmake-debug --parallel` | `meson compile -C build/meson-debug` |
| **Test** | `ctest --test-dir build/cmake-debug --output-on-failure` | `meson test -C build/meson-debug --suite PAOO --print-errorlogs` |
| **Run** | `build/cmake-debug/src/paoo` | `build/meson-debug/src/paoo` |
