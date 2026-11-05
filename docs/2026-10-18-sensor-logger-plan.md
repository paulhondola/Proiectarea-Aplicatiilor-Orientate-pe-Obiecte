> **Superseded:** this document describes the original `SensorLogger` topic. See [the image filter design](2026-11-05-image-filter-pipeline-design.md).

# Lab 03 `SensorLogger` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a standalone C++14 `SensorLogger` application with CMake that demonstrates encapsulation, constructor member initializer lists, heap cleanup in the destructor, deep copy construction (vs. shallow copy bugs), and move construction.

**Architecture:** A static library `sensor_lib` (`inc/SensorLogger.hpp`, `src/SensorLogger.cpp`) encapsulates a heap-allocated `double* readings` array and `const int sensorId`. An executable target `Lab_03` (`src/main.cpp`) links against `sensor_lib` and runs 4 self-verifying console demonstrations covering all grading targets in `docs/task.md`.

**Tech Stack:** C++14, CMake (>= 3.10), standard library (`<iostream>`, `<string>`, `<utility>`, `<cassert>`).

**Spec:** `Lab/Paul/Lab_03/docs/2026-10-18-sensor-logger-design.md`

## Global Constraints

- Minimum CMake version: `cmake_minimum_required(VERSION 3.10)` with `project(Lab_03)`.
- C++ Standard: `CMAKE_CXX_STANDARD 14`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`.
- Directory layout must strictly use `inc/` for headers and `src/` for `.cpp` files inside `Lab/Paul/Lab_03`.
- Header `inc/SensorLogger.hpp` must use `#ifndef SENSOR_LOGGER_HPP` / `#define SENSOR_LOGGER_HPP` / `#endif` include guards.
- All `SensorLogger` data members (`sensorId`, `location`, `capacity`, `count`, `readings`) must be `private`.
- Every constructor and destructor must log its invocation and the heap pointer address (`static_cast<const void*>(readings)`) to `std::cout`.

## Review Focus

1. **Non-positive capacity (`cap <= 0`) passed to constructor:** Clamped to `1` in the member initializer list (`cap > 0 ? cap : 1`) so `new double[capacity]` always allocates a valid buffer.
2. **Adding readings beyond `capacity`:** `addReading` checks `count >= capacity`, prints a warning, leaves the heap buffer untouched, and returns `false`.
3. **Out-of-bounds index in `updateReading` or `getReading`:** Checks `index < 0 || index >= count` and returns `false` (or `0.0` for `getReading`) without reading/writing out-of-bounds memory.
4. **Accessing or destructing a moved-from `SensorLogger`:** Move constructor sets `other.readings = nullptr`, `other.capacity = 0`, `other.count = 0`; `printStatus` and `~SensorLogger()` safely handle `readings == nullptr`.
5. **Empty string passed to `setLocation("")`:** Ignored when `loc.empty()` so `location` never becomes blank.

---

### Task 1: CMake Build & `SensorLogger` Static Library

**Files:**
- Modify: `Lab/Paul/Lab_03/CMakeLists.txt`
- Create: `Lab/Paul/Lab_03/inc/SensorLogger.hpp`
- Create: `Lab/Paul/Lab_03/src/SensorLogger.cpp`
- Create: `Lab/Paul/Lab_03/src/main.cpp` (initial smoke-test harness)

**Interfaces:**
- Produces in `inc/SensorLogger.hpp`:
  - `class SensorLogger` with:
    - `SensorLogger(int id, const std::string& loc, int cap)`
    - `~SensorLogger()`
    - `SensorLogger(const SensorLogger& other)`
    - `SensorLogger(SensorLogger&& other) noexcept`
    - `bool addReading(double value)`
    - `bool updateReading(int index, double value)`
    - `double getReading(int index) const`
    - `int getId() const`
    - `const std::string& getLocation() const`
    - `void setLocation(const std::string& loc)`
    - `int getCount() const`
    - `int getCapacity() const`
    - `const double* getRawBufferAddress() const`
    - `void printStatus(const std::string& label) const`

- [ ] **Step 1: Configure `Lab/Paul/Lab_03/CMakeLists.txt` with `sensor_lib` and `Lab_03` targets**

Replace the placeholder `CMakeLists.txt` with:
- `cmake_minimum_required(VERSION 3.10)` and `project(Lab_03)`
- C++14 standard settings (`CMAKE_CXX_STANDARD 14`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`)
- `add_library(sensor_lib STATIC src/SensorLogger.cpp inc/SensorLogger.hpp)`
- `target_include_directories(sensor_lib PUBLIC inc)`
- `add_executable(${PROJECT_NAME} src/main.cpp)`
- `target_link_libraries(${PROJECT_NAME} PUBLIC sensor_lib)`

- [ ] **Step 2: Write unit/smoke test in `Lab/Paul/Lab_03/src/main.cpp` exercising `SensorLogger` lifecycle & edge cases**

```cpp
#include "SensorLogger.hpp"
#include <cassert>
#include <utility>

int main() {
    // 1. Clamped non-positive capacity + initializer list
    SensorLogger clamped(1, "Test", 0);
    assert(clamped.getId() == 1);
    assert(clamped.getCapacity() == 1);
    assert(clamped.getCount() == 0);
    assert(clamped.getRawBufferAddress() != nullptr);

    // 2. Encapsulation & bounds checks
    assert(clamped.addReading(21.5) == true);
    assert(clamped.addReading(22.0) == false); // capacity full
    assert(clamped.updateReading(0, 25.0) == true);
    assert(clamped.updateReading(-1, 10.0) == false);
    assert(clamped.updateReading(5, 10.0) == false);
    assert(clamped.getReading(0) == 25.0);
    clamped.setLocation("");
    assert(clamped.getLocation() == "Test");

    // 3. Deep copy independence
    SensorLogger copy(clamped);
    assert(copy.getRawBufferAddress() != clamped.getRawBufferAddress());
    assert(copy.getReading(0) == 25.0);
    copy.updateReading(0, 99.0);
    assert(clamped.getReading(0) == 25.0);

    // 4. Move constructor pointer theft & source reset
    const double* originalPtr = copy.getRawBufferAddress();
    SensorLogger moved(std::move(copy));
    assert(moved.getRawBufferAddress() == originalPtr);
    assert(copy.getRawBufferAddress() == nullptr);
    assert(copy.getCount() == 0);
    assert(copy.getCapacity() == 0);

    return 0;
}
```

- [ ] **Step 3: Run CMake build to verify it fails before implementation**

Run in `Lab/Paul/Lab_03`: `cmake -S . -B build`
Expected: FAIL because `inc/SensorLogger.hpp` and `src/SensorLogger.cpp` do not exist yet.

- [ ] **Step 4: Implement `Lab/Paul/Lab_03/inc/SensorLogger.hpp` and `Lab/Paul/Lab_03/src/SensorLogger.cpp`**

Implement `SensorLogger` matching the exact signatures in **Interfaces** and rules in **Global Constraints** & **Review Focus**:
- Constructor uses member initializer list `: sensorId(id), location(loc), capacity(cap > 0 ? cap : 1), count(0), readings(new double[capacity])` and zero-initializes `readings`.
- Copy constructor allocates `new double[other.capacity]` and copies `other.count` elements.
- Move constructor (`noexcept`) steals `other.readings` and sets `other.readings = nullptr`, `other.capacity = 0`, `other.count = 0`.
- Destructor logs pointer address (or `nullptr`) and calls `delete[] readings;`.

- [ ] **Step 5: Build and run the verification binary**

Run in `Lab/Paul/Lab_03`: `cmake -S . -B build && cmake --build build && ./build/Lab_03`
Expected: Compiles cleanly with zero errors and `./build/Lab_03` exits with code `0` (all assertions pass).

---

### Task 2: Complete Interactive Demonstration Scenarios in `src/main.cpp`

**Files:**
- Modify: `Lab/Paul/Lab_03/src/main.cpp`

**Interfaces:**
- Consumes: `SensorLogger` from `inc/SensorLogger.hpp`
- Produces: Complete 4-part console demonstration binary `./build/Lab_03` covering all targets in `Lab/Paul/Lab_03/docs/task.md`.

- [ ] **Step 1: Implement `ShallowSensorDemo` and the 4 labeled demo functions in `Lab/Paul/Lab_03/src/main.cpp`**

Replace `src/main.cpp` with the 4 structured demonstrations specified in Section 4 of the spec, keeping `assert(...)` checks inline so the demonstration also self-tests every requirement:
- `struct ShallowSensorDemo`: allocates `new double[2]` in constructor, uses compiler-generated default copy constructor, frees `delete[] data` in destructor.
- **Demo 1 (Targets 2 & 3):** Instantiates `SensorLogger roomSensor(101, "ServerRoom-A", 3)`, prints explanation of `const int sensorId` member initializer list requirement, records readings (`22.5`, `23.0`, `24.1`), tests capacity limit (`addReading(25.0)` returns `false`) and out-of-bounds protection (`updateReading(99, 50.0)` returns `false`), and tests `setLocation("")` rejection.
- **Demo 2 (Target 4):** Creates `SensorLogger hallwaySensor(102, "Lab-Hallway", 2)` inside an inner `{ ... }` scope and logs automatic heap cleanup when the scope exits.
- **Demo 3 (Target 5):**
  - **Part A (Problem):** Copies `ShallowSensorDemo badCopy = badOrig;`, asserts `badCopy.data == badOrig.data`, mutates `badCopy.data[0] = 999.9`, asserts `badOrig.data[0] == 999.9` (proving silent corruption), then sets `badCopy.data = nullptr` with an explanation of the double-free `SIGABRT` prevented.
  - **Part B (Fix):** Copies `SensorLogger backupSensor(roomSensor);`, asserts `backupSensor.getRawBufferAddress() != roomSensor.getRawBufferAddress()`, mutates `backupSensor.updateReading(0, 99.9)`, and asserts `roomSensor.getReading(0) == 22.5`.
- **Demo 4 (Target 6):** Creates `SensorLogger stagedSensor(103, "Roof-Weather", 3)`, saves `const double* originalHeapPtr = stagedSensor.getRawBufferAddress()`, moves into `SensorLogger archivedSensor(std::move(stagedSensor))`, and asserts `archivedSensor.getRawBufferAddress() == originalHeapPtr` and `stagedSensor.getRawBufferAddress() == nullptr`.

- [ ] **Step 2: Build and run `./build/Lab_03` and verify output**

Run in `Lab/Paul/Lab_03`: `cmake --build build && ./build/Lab_03 && rm -rf build`
Expected: All 4 demos print clear lifecycle logs (`[Constructor]`, `[Copy Constructor]`, `[Move Constructor]`, `[Destructor]`) with heap addresses, all assertions pass with exit code `0`, and temporary `build/` directory is cleaned up before finishing.
