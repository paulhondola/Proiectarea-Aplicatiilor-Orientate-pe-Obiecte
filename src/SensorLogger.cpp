#include "SensorLogger.hpp"
#include <iostream>
#include <utility>

SensorLogger::SensorLogger(int id, const std::string& loc, int cap)
    : sensorId(id),
      location(loc),
      capacity(cap > 0 ? cap : 1),
      count(0),
      readings(new double[capacity]) {
    for (int i = 0; i < capacity; ++i) {
        readings[i] = 0.0;
    }
    std::cout << "  [Constructor] Sensor #" << sensorId << " (" << location
              << ") allocated heap buffer at "
              << static_cast<const void*>(readings)
              << " (capacity=" << capacity << ")\n";
}

SensorLogger::~SensorLogger() {
    if (readings != nullptr) {
        std::cout << "  [Destructor]  Sensor #" << sensorId << " (" << location
                  << ") freeing active heap buffer at "
                  << static_cast<const void*>(readings) << "\n";
    } else {
        std::cout << "  [Destructor]  Sensor #" << sensorId
                  << " (moved-from) has nullptr buffer — nothing to free\n";
    }
    delete[] readings;
}

SensorLogger::SensorLogger(const SensorLogger& other)
    : sensorId(other.sensorId),
      location(other.location + " [Copy]"),
      capacity(other.capacity),
      count(other.count),
      readings(other.capacity > 0 ? new double[other.capacity] : nullptr) {
    for (int i = 0; i < capacity; ++i) {
        readings[i] = (i < count && other.readings != nullptr) ? other.readings[i] : 0.0;
    }
    std::cout << "  [Copy Ctor]   Sensor #" << sensorId
              << " deep-copied buffer from "
              << static_cast<const void*>(other.readings) << " -> NEW heap buffer at "
              << static_cast<const void*>(readings) << "\n";
}

SensorLogger::SensorLogger(SensorLogger&& other) noexcept
    : sensorId(other.sensorId),
      location(std::move(other.location)),
      capacity(other.capacity),
      count(other.count),
      readings(other.readings) {
    other.readings = nullptr;
    other.capacity = 0;
    other.count = 0;
    std::cout << "  [Move Ctor]   Sensor #" << sensorId << " (" << location
              << ") stole heap buffer at "
              << static_cast<const void*>(readings)
              << " (source pointer reset to nullptr)\n";
}

bool SensorLogger::addReading(double value) {
    if (readings == nullptr || count >= capacity) {
        std::cout << "  [Encapsulation Guard] Cannot add reading " << value
                  << ": buffer full or moved-from (count=" << count
                  << ", capacity=" << capacity << ")\n";
        return false;
    }
    readings[count++] = value;
    return true;
}

bool SensorLogger::updateReading(int index, double value) {
    if (readings == nullptr || index < 0 || index >= count) {
        std::cout << "  [Encapsulation Guard] Out-of-bounds update rejected for index "
                  << index << " (valid range: 0.." << (count - 1) << ")\n";
        return false;
    }
    readings[index] = value;
    return true;
}

double SensorLogger::getReading(int index) const {
    if (readings == nullptr || index < 0 || index >= count) {
        return 0.0;
    }
    return readings[index];
}

int SensorLogger::getId() const {
    return sensorId;
}

const std::string& SensorLogger::getLocation() const {
    return location;
}

void SensorLogger::setLocation(const std::string& loc) {
    if (loc.empty()) {
        std::cout << "  [Encapsulation Guard] Ignored empty location string.\n";
        return;
    }
    location = loc;
}

int SensorLogger::getCount() const {
    return count;
}

int SensorLogger::getCapacity() const {
    return capacity;
}

const double* SensorLogger::getRawBufferAddress() const {
    return readings;
}

void SensorLogger::printStatus(const std::string& label) const {
    std::cout << "  -> " << label << " | ID: " << sensorId
              << " | Location: \"" << location << "\""
              << " | Heap Ptr: " << static_cast<const void*>(readings)
              << " | Readings (" << count << "/" << capacity << "): [";
    if (readings != nullptr) {
        for (int i = 0; i < count; ++i) {
            std::cout << readings[i] << (i + 1 < count ? ", " : "");
        }
    } else {
        std::cout << "moved-from / nullptr";
    }
    std::cout << "]\n";
}
