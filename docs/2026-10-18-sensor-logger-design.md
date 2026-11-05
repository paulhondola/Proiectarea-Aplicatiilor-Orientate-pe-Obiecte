> **Superseded:** this document describes the original `SensorLogger` topic. See [the image filter design](2026-11-05-image-filter-pipeline-design.md).

# Lab 03 Design Spec — `SensorLogger` C++ Application

**Date:** 2026-10-18  
**Scope:** Individual standalone C++14 project for `Lab/Paul/Lab_03`

---

## 1. Purpose & Requirements Mapping

Build a minimal, self-contained C++ application (`SensorLogger`) that fulfills all 6 grading targets in `Lab/Paul/Lab_03/docs/task.md` and prints clear console demonstrations of why each OOP concept is needed:

| # | Requirement (`task.md`) | Points | Design Coverage |
|---|---|---|---|
| 1 | Build using **CMake** following the lab structure (`inc/`, `src/`, static lib + executable) | 3p | `CMakeLists.txt` compiles `src/SensorLogger.cpp` into static library `sensor_lib` with public headers in `inc/`, and links it into executable `Lab_03` (`src/main.cpp`). |
| 2 | **Encapsulation** | 1p | All data members (`sensorId`, `location`, `capacity`, `count`, `readings`) are `private`. Access and mutation are strictly controlled via `public` methods with bounds checking. |
| 3 | **Member initialization in constructor** | 1p | Constructor uses member initializer list `: sensorId(id), location(loc), capacity(cap), count(0), readings(new double[cap])`. `const int sensorId` proves why initializer lists are required by C++. |
| 4 | **Freeing heap in destructor** | 1p | `~SensorLogger()` releases heap memory with `delete[] readings;` and logs the exact freed pointer address (`0x...` or `nullptr`). |
| 5 | **Overriding copy constructor** | 2p | `SensorLogger(const SensorLogger& other)` performs a deep copy (`new double[other.capacity]`). Contrasted in `main.cpp` against `ShallowSensorDemo`, which uses the default shallow copy and suffers from shared-pointer data corruption and double-free risk. |
| 6 | **Creating a move constructor** | 2p | `SensorLogger(SensorLogger&& other) noexcept` transfers ownership of `other.readings` in $O(1)$ without reallocating and sets `other.readings = nullptr`, `other.count = 0`, `other.capacity = 0`. |

---

## 2. Project File Structure

```text
Lab/Paul/Lab_03/
├── CMakeLists.txt
├── docs/
│   ├── task.md
│   └── 2026-10-18-sensor-logger-design.md
├── inc/
│   └── SensorLogger.hpp
└── src/
    ├── SensorLogger.cpp
    └── main.cpp
```

### `CMakeLists.txt` Design
- `cmake_minimum_required(VERSION 3.10)`
- `project(Lab_03)`
- Enforces standard C++14 (`CMAKE_CXX_STANDARD 14`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`).
- Defines static library `sensor_lib` (`add_library(sensor_lib STATIC src/SensorLogger.cpp inc/SensorLogger.hpp)`).
- Exposes `inc/` via `target_include_directories(sensor_lib PUBLIC inc)`.
- Defines executable `Lab_03` (`add_executable(${PROJECT_NAME} src/main.cpp)`) and links `sensor_lib` (`target_link_libraries(${PROJECT_NAME} PUBLIC sensor_lib)`).

---

## 3. Component Design

### 3.1 `SensorLogger` Class (`inc/SensorLogger.hpp`, `src/SensorLogger.cpp`)

Protected by `#ifndef SENSOR_LOGGER_HPP` / `#define SENSOR_LOGGER_HPP` / `#endif` include guards.

#### Private Data Members
- `const int sensorId;` — Immutable sensor identifier; requires initialization in the constructor's member initializer list.
- `std::string location;` — Human-readable sensor placement label.
- `int capacity;` — Maximum number of readings allocated in the heap buffer.
- `int count;` — Number of readings currently recorded (`0 <= count <= capacity`).
- `double* readings;` — Raw pointer to heap-allocated array (`new double[capacity]`).

#### Public Constructors & Destructor
1. **Parameterized Constructor:** `SensorLogger(int id, const std::string& loc, int cap)`
   - Uses member initializer list: `: sensorId(id), location(loc), capacity(cap > 0 ? cap : 1), count(0), readings(new double[capacity])`.
   - Zero-initializes the allocated array elements and prints `[Constructor]` with `sensorId`, `location`, and heap address `static_cast<const void*>(readings)`.
2. **Destructor:** `~SensorLogger()`
   - Prints `[Destructor]` with `sensorId`, `location`, and the pointer address being freed (noting whether it is an active heap buffer or `nullptr` after a move).
   - Executes `delete[] readings;`.
3. **Copy Constructor (Deep Copy):** `SensorLogger(const SensorLogger& other)`
   - Uses member initializer list: `: sensorId(other.sensorId), location(other.location + " (Copy)"), capacity(other.capacity), count(other.count), readings(new double[other.capacity])`.
   - Copies all `count` values from `other.readings` into the newly allocated `readings` buffer.
   - Prints `[Copy Constructor]` showing both the source heap address and the new deep-copied heap address.
4. **Move Constructor:** `SensorLogger(SensorLogger&& other) noexcept`
   - Uses member initializer list: `: sensorId(other.sensorId), location(std::move(other.location)), capacity(other.capacity), count(other.count), readings(other.readings)`.
   - Nulls out the source object: `other.readings = nullptr; other.capacity = 0; other.count = 0;`.
   - Prints `[Move Constructor]` showing that ownership of the heap address was transferred without new allocation.

#### Public Encapsulated Methods
- `bool addReading(double value)` — Appends `value` if `count < capacity`; otherwise prints a capacity warning and returns `false`.
- `bool updateReading(int index, double value)` — Updates `readings[index]` if `0 <= index < count`; otherwise prints an out-of-bounds error and returns `false`.
- `int getId() const` — Returns `sensorId`.
- `const std::string& getLocation() const` — Returns `location`.
- `void setLocation(const std::string& loc)` — Updates `location` if non-empty.
- `int getCount() const` / `int getCapacity() const` — Returns current `count` / `capacity`.
- `void printStatus(const std::string& label) const` — Prints object label, `sensorId`, `location`, heap pointer address (`readings`), and all recorded values.

---

### 3.2 `ShallowSensorDemo` Helper (`src/main.cpp`)

To concretely demonstrate **why** overriding the copy constructor is required (Target 5) without crashing the program:
- A minimal struct inside `src/main.cpp` holding `double* data` allocated with `new double[2]`, using the **compiler-generated default copy constructor** (shallow copy).
- In Demo 3A, `ShallowSensorDemo shallowCopy = shallowOriginal;` copies only the pointer value (`shallowCopy.data == shallowOriginal.data`).
- Modifying `shallowCopy.data[0] = 999.9` overwrites `shallowOriginal.data[0]`, proving unintended aliasing.
- Before scope exit, `shallowCopy.data = nullptr;` is executed explicitly with a console explanation that without this manual neutralization, `~ShallowSensorDemo()` would run `delete[]` twice on the same address (Undefined Behavior / `SIGABRT` double-free crash).

---

## 4. Demonstration Scenarios (`src/main.cpp`)

`main()` executes 4 sequential, clearly formatted console sections:

1. **Demo 1 — Encapsulation & Constructor Member Initializer List (Targets 2 & 3):**
   - Instantiates `SensorLogger roomSensor(101, "ServerRoom-A", 3)`.
   - Explains why `: sensorId(id), ...` is mandatory for `const int sensorId` and avoids default-construct + reassign overhead on `std::string location`.
   - Adds valid readings and triggers encapsulated validation errors (`updateReading(10, 50.0)` and exceeding capacity) to prove private state cannot be corrupted.
2. **Demo 2 — Automatic Heap Cleanup via Destructor (Target 4):**
   - Creates a scoped `SensorLogger hallwaySensor(102, "Lab-Hallway", 2)` inside an inner `{ ... }` block.
   - Shows the heap allocation address on entry and immediate `delete[]` in `~SensorLogger()` at the end of the block.
3. **Demo 3 — Shallow Copy Problem vs. Overridden Deep Copy Constructor (Target 5):**
   - **3A (Problem):** Runs `ShallowSensorDemo` to show identical heap addresses (`0x... == 0x...`) and silent data corruption when the copy is mutated.
   - **3B (Fix):** Clones `SensorLogger backupSensor(roomSensor);`, prints distinct heap addresses (`0x... != 0x...`), mutates `backupSensor`, and verifies `roomSensor` is untouched.
4. **Demo 4 — Move Constructor Efficiency & Ownership Transfer (Target 6):**
   - Creates `SensorLogger stagedSensor(103, "Roof-Weather", 3)` and moves it into `SensorLogger archivedSensor(std::move(stagedSensor))`.
   - Verifies that `archivedSensor` now owns the exact same heap pointer `0x...` (zero new heap allocations), while `stagedSensor` holds `nullptr` (`0x0`) and destructs safely.

---

## 5. Build & Verification Criteria

- Configure: `cmake -S . -B build` inside `Lab/Paul/Lab_03` completes without warnings or errors.
- Build: `cmake --build build` compiles `libsensor_lib.a` and links `Lab_03`.
- Run: `./build/Lab_03` executes all 4 demos, logs every constructor and destructor call with heap addresses, and exits with code `0`.
