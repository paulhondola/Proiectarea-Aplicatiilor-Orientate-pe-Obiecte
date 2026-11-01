#include "SensorLogger.hpp"
#include <cassert>
#include <iostream>
#include <utility>

// ============================================================================
// Helper struct WITHOUT a custom Copy Constructor (uses compiler default
// shallow copy) to concretely illustrate the problem in Target 5.
// ============================================================================
struct ShallowSensorDemo {
    int id;
    double* data;

    ShallowSensorDemo(int sensorId, double initialVal)
        : id(sensorId), data(new double[2]) {
        data[0] = initialVal;
        data[1] = 0.0;
        std::cout << "  [ShallowSensorDemo Ctor] Allocated heap at "
                  << static_cast<const void*>(data) << "\n";
    }

    // Compiler generates default shallow copy constructor:
    // ShallowSensorDemo(const ShallowSensorDemo& other) = default;

    ~ShallowSensorDemo() {
        if (data != nullptr) {
            std::cout << "  [ShallowSensorDemo Dtor] Freeing heap at "
                      << static_cast<const void*>(data) << "\n";
        } else {
            std::cout << "  [ShallowSensorDemo Dtor] Pointer neutralized to "
                         "nullptr to avoid double-free crash (SIGABRT)\n";
        }
        delete[] data;
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "   LAB 03 - OOP C++: SensorLogger Lifecycle Demonstration   \n";
    std::cout << "============================================================\n\n";

    // ------------------------------------------------------------------------
    // DEMO 1: Encapsulation (Target 2) & Member Initializer List (Target 3)
    // ------------------------------------------------------------------------
    std::cout << "--- DEMO 1: Encapsulation & Constructor Initializer List ---\n";
    std::cout << "Why member initializer list (: sensorId(id), location(loc), ...)?\n"
              << "  1. 'const int sensorId' MUST be initialized before the constructor\n"
              << "     body starts (assigning inside '{ ... }' is a compile error).\n"
              << "  2. 'std::string location' is constructed directly in one step\n"
              << "     instead of default-constructing + reassigning.\n\n";

    SensorLogger roomSensor(101, "ServerRoom-A", 3);
    assert(roomSensor.getId() == 101);
    assert(roomSensor.getCapacity() == 3);
    assert(roomSensor.getCount() == 0);

    // Record valid readings via encapsulated public interface
    assert(roomSensor.addReading(22.5) == true);
    assert(roomSensor.addReading(23.0) == true);
    assert(roomSensor.addReading(24.1) == true);
    roomSensor.printStatus("roomSensor (after 3 readings)");

    // Demonstrate encapsulation guards protecting private state & heap buffer
    std::cout << "\nTesting Encapsulation Guards (preventing invalid state):\n";
    assert(roomSensor.addReading(25.0) == false);       // Exceeds capacity (3)
    assert(roomSensor.updateReading(99, 50.0) == false); // Out-of-bounds index
    assert(roomSensor.updateReading(-1, 50.0) == false); // Negative index
    roomSensor.setLocation("");                          // Empty string rejected
    assert(roomSensor.getLocation() == "ServerRoom-A");

    // Verify edge-case clamping for non-positive capacity
    {
        SensorLogger clampedSensor(100, "Clamped-Test", 0);
        assert(clampedSensor.getCapacity() == 1);
    }
    std::cout << "\n";

    // ------------------------------------------------------------------------
    // DEMO 2: Freeing Heap Memory in Destructor (Target 4)
    // ------------------------------------------------------------------------
    std::cout << "--- DEMO 2: Automatic Heap Deallocation in Destructor ---\n";
    std::cout << "Why destructor cleanup (delete[] readings)?\n"
              << "  Without delete[] in ~SensorLogger(), every object going out\n"
              << "  of scope leaks its heap-allocated double[] array.\n\n";
    {
        std::cout << "Entering inner scope {\n";
        SensorLogger hallwaySensor(102, "Lab-Hallway", 2);
        hallwaySensor.addReading(19.8);
        hallwaySensor.addReading(20.1);
        hallwaySensor.printStatus("hallwaySensor (inside scope)");
        std::cout << "} Leaving inner scope -> destructor called automatically:\n";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------------
    // DEMO 3: Why Override the Copy Constructor? Shallow vs. Deep Copy (Target 5)
    // ------------------------------------------------------------------------
    std::cout << "--- DEMO 3: Copy Constructor (Shallow Copy Bug vs. Deep Copy) ---\n";
    std::cout << "[Part A] PROBLEM without custom Copy Constructor (Shallow Copy):\n";
    {
        ShallowSensorDemo badOrig(1, 22.5);
        ShallowSensorDemo badCopy = badOrig; // Default shallow copy!

        std::cout << "  -> badOrig.data ptr: " << static_cast<const void*>(badOrig.data)
                  << " | value[0] = " << badOrig.data[0] << "\n";
        std::cout << "  -> badCopy.data ptr: " << static_cast<const void*>(badCopy.data)
                  << " | value[0] = " << badCopy.data[0] << "\n";

        // Both objects point to the EXACT SAME heap address!
        assert(badOrig.data == badCopy.data);

        std::cout << "  Mutating ONLY badCopy.data[0] = 999.9...\n";
        badCopy.data[0] = 999.9;
        std::cout << "  -> badOrig.data[0] is now: " << badOrig.data[0]
                  << " (SILENTLY CORRUPTED!)\n";
        assert(badOrig.data[0] == 999.9);

        // Neutralize badCopy.data before scope exit so our demo doesn't abort
        // with SIGABRT when both destructors run delete[] on the same pointer.
        badCopy.data = nullptr;
    }

    std::cout << "\n[Part B] FIX with overridden Copy Constructor (Deep Copy):\n";
    {
        SensorLogger backupSensor = roomSensor; // Calls custom Copy Constructor

        // Deep copy allocates a DISTINCT heap buffer
        assert(backupSensor.getRawBufferAddress() != roomSensor.getRawBufferAddress());
        assert(backupSensor.getReading(0) == 22.5);

        std::cout << "  Mutating backupSensor reading[0] to 99.9...\n";
        assert(backupSensor.updateReading(0, 99.9) == true);

        roomSensor.printStatus("roomSensor   (original - untouched)");
        backupSensor.printStatus("backupSensor (deep copy - modified)");

        assert(roomSensor.getReading(0) == 22.5);
        assert(backupSensor.getReading(0) == 99.9);
    }
    std::cout << "\n";

    // ------------------------------------------------------------------------
    // DEMO 4: Move Constructor & Ownership Transfer (Target 6)
    // ------------------------------------------------------------------------
    std::cout << "--- DEMO 4: Move Constructor (O(1) Ownership Transfer) ---\n";
    std::cout << "Why Move Constructor (SensorLogger(SensorLogger&& other) noexcept)?\n"
              << "  When transferring a temporary/staged object, allocating a new\n"
              << "  heap buffer and copying elements is wasted work. The move\n"
              << "  constructor steals the existing heap pointer in O(1) and sets\n"
              << "  the source pointer to nullptr.\n\n";
    {
        SensorLogger stagedSensor(103, "Roof-Weather", 3);
        stagedSensor.addReading(14.2);
        stagedSensor.addReading(15.0);
        const double* originalHeapPtr = stagedSensor.getRawBufferAddress();

        stagedSensor.printStatus("stagedSensor (before std::move)");

        // Transfer ownership using Move Constructor
        SensorLogger archivedSensor(std::move(stagedSensor));

        // Verify archivedSensor stole the exact heap address and stagedSensor is nulled
        assert(archivedSensor.getRawBufferAddress() == originalHeapPtr);
        assert(stagedSensor.getRawBufferAddress() == nullptr);
        assert(stagedSensor.getCount() == 0);
        assert(stagedSensor.getCapacity() == 0);
        assert(archivedSensor.getReading(0) == 14.2);

        stagedSensor.printStatus("stagedSensor   (after std::move)");
        archivedSensor.printStatus("archivedSensor (new owner)");
        std::cout << "Leaving Demo 4 scope (both destructors run safely):\n";
    }

    std::cout << "\n============================================================\n";
    std::cout << " All 4 demonstrations & assertions completed successfully!  \n";
    std::cout << " Cleaning up main scope objects:                            \n";
    std::cout << "============================================================\n";
    return 0;
}
