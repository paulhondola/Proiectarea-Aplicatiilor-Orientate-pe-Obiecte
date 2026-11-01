#ifndef SENSOR_LOGGER_HPP
#define SENSOR_LOGGER_HPP

#include <string>

class SensorLogger {
private:
    const int sensorId;
    std::string location;
    int capacity;
    int count;
    double* readings;

public:
    // 1. Parameterized constructor using member initializer list
    SensorLogger(int id, const std::string& loc, int cap);

    // 2. Destructor freeing heap memory
    ~SensorLogger();

    // 3. Copy constructor (deep copy)
    SensorLogger(const SensorLogger& other);

    // 4. Move constructor (stealing heap pointer)
    SensorLogger(SensorLogger&& other) noexcept;

    // Encapsulated methods
    bool addReading(double value);
    bool updateReading(int index, double value);
    double getReading(int index) const;

    int getId() const;
    const std::string& getLocation() const;
    void setLocation(const std::string& loc);
    int getCount() const;
    int getCapacity() const;
    const double* getRawBufferAddress() const;

    void printStatus(const std::string& label) const;
};

#endif // SENSOR_LOGGER_HPP
