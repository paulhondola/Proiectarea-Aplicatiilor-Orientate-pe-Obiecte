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
        : name(presetName), radius(presetRadius) {}

    const std::string name;
    const int radius;
};

#endif // DEMO_TYPES_HPP
