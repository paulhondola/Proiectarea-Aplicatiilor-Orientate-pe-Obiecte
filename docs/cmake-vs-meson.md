# CMake vs. Meson Comparison (`Lab_03`)

This note compares [`CMakeLists.txt`](../CMakeLists.txt) and [`meson.build`](../meson.build) in `Lab/Paul/Lab_03`. Both build systems compile the exact same C++14 sources (`src/SensorLogger.cpp`, `src/main.cpp`, `inc/SensorLogger.hpp`) into the `sensor_lib` static library and `Lab_03` executable without modifying any project code.

---

## 1. Side-by-Side Build Definitions

### CMake (`CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.10)

project(Lab_03)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sensor_lib STATIC
    src/SensorLogger.cpp
    inc/SensorLogger.hpp
)

target_include_directories(sensor_lib PUBLIC inc)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC sensor_lib)
```

### Meson (`meson.build`)
```meson
project(
  'Lab_03',
  'cpp',
  version : '1.0.0',
  meson_version : '>= 0.56.0',
  default_options : [
    'cpp_std=c++14',
    'warning_level=2',
  ],
)

inc_dir = include_directories('inc')

sensor_lib = static_library(
  'sensor_lib',
  'src/SensorLogger.cpp',
  include_directories : inc_dir,
)

sensor_dep = declare_dependency(
  link_with : sensor_lib,
  include_directories : inc_dir,
)

lab_exe = executable(
  meson.project_name(),
  'src/main.cpp',
  dependencies : [sensor_dep],
)

test('sensor_logger_lifecycle', lab_exe)
```

---

## 2. Key Architectural Differences

| Aspect | CMake | Meson |
| :--- | :--- | :--- |
| **Language & Paradigm** | Turing-complete scripting language with mutable global state and string expansion (`${PROJECT_NAME}`). | Non-Turing-complete, strongly-typed declarative DSL (no arbitrary user functions or global variable mutation side effects). |
| **C++ Standard Setup** | Requires 3 imperative variable assignments (`CMAKE_CXX_STANDARD`, `CMAKE_CXX_STANDARD_REQUIRED`, `CMAKE_CXX_EXTENSIONS`). | Single declarative option `cpp_std=c++14` in `project(..., default_options: ...)`. Choosing `c++14` instead of `gnu++14` disables compiler extensions automatically. |
| **Referencing Targets** | Targets are looked up by **string identifier** (`sensor_lib`, `${PROJECT_NAME}`). Typos in target names can sometimes be treated as raw linker flags (`-lsensor_lib`). | Functions return **first-class typed objects** (`inc_dir`, `sensor_lib`, `sensor_dep`, `lab_exe`). Referencing an undefined variable is an immediate parse error. |
| **Transitive Headers (`PUBLIC` vs `declare_dependency`)** | `target_include_directories(sensor_lib PUBLIC inc)` mutates `sensor_lib` in place so any target calling `target_link_libraries(... sensor_lib)` automatically gets `-Iinc`. | `include_directories` on `static_library()` is **private** to building that library. To expose headers transitively like CMake's `PUBLIC`, Meson wraps the library and include object in `declare_dependency()`. |
| **Out-of-Source Builds** | Allows in-source builds (`cmake .`) unless explicitly blocked, which can pollute the repository tree. | Enforces out-of-source builds by design (`meson setup <builddir>` refuses to run without a separate build directory). |
| **Default Backend** | Defaults to Unix Makefiles on Linux/macOS unless `-G Ninja` is specified. | Defaults to **Ninja** everywhere for fast incremental builds. |

---

## 3. CLI Workflow Comparison

| Step | CMake Command | Meson Command |
| :--- | :--- | :--- |
| **1. Configure** | `cmake -S . -B build` | `meson setup build_meson` |
| **2. Compile** | `cmake --build build` | `meson compile -C build_meson` |
| **3. Run Binary** | `./build/Lab_03` | `./build_meson/Lab_03` |
| **4. Run Tests** | `ctest --test-dir build` *(if `enable_testing()` added)* | `meson test -C build_meson -v` |
