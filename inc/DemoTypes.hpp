#ifndef DEMO_TYPES_HPP
#define DEMO_TYPES_HPP

#include <string>

// Owns a buffer but relies on the generated copy constructor, which copies only
// the pointer. Whoever copies one must neutralize a pointer before scope exit,
// or both destructors free the same buffer.
struct ShallowImage {
    unsigned char* data;

    explicit ShallowImage(unsigned char value) : data(new unsigned char[1]) {
        data[0] = value;
    }
    ~ShallowImage() {
        delete[] data;
    }
};

// Its const members can only be set in the constructor's initializer list.
class FilterPreset {
public:
    FilterPreset(const std::string& presetName, int presetRadius)
        : name(presetName), radius(presetRadius) {
    }

    const std::string name;
    const int radius;
};

// Counts its operations, to show what a class really does to its members.
struct Probe {
    inline static int defaultConstructions = 0;
    inline static int copyConstructions = 0;
    inline static int copyAssignments = 0;

    Probe() {
        ++defaultConstructions;
    }
    Probe(const Probe&) {
        ++copyConstructions;
    }
    Probe& operator=(const Probe&) {
        ++copyAssignments;
        return *this;
    }

    static void reset() {
        defaultConstructions = 0;
        copyConstructions = 0;
        copyAssignments = 0;
    }
};

// Item 4: initialization order across source files. The definitions are in
// lab4_order_a.cpp and lab4_order_b.cpp.
constexpr int orderExpectedValue = 42;
int orderDynamicValue() noexcept;
extern int orderGlobalValue;
int& orderLocalValue() noexcept;
extern int orderCopiedAtStartup;
extern int orderLocalAtStartup;

#endif // DEMO_TYPES_HPP
