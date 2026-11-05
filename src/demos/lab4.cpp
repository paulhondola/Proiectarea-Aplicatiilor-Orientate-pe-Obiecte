#include "DemoTypes.hpp"
#include "Demos.hpp"
#include "Image.hpp"
#include "Logger.hpp"

#include <iostream>
#include <type_traits>

// Item 5: everything the compiler writes for a class that declares nothing.
struct PlainStruct {
    int id = 0;
    Probe probe;
};
static_assert(std::is_default_constructible<PlainStruct>::value, "compiler generates a default");
static_assert(std::is_copy_constructible<PlainStruct>::value, "compiler generates a copy");
static_assert(std::is_copy_assignable<PlainStruct>::value, "compiler generates operator=");
static_assert(std::is_destructible<PlainStruct>::value, "compiler generates a destructor");

// Item 5: declaring any constructor removes the generated default constructor.
struct HasOwnConstructor {
    int value;
    explicit HasOwnConstructor(int newValue) : value(newValue) {
    }
};
static_assert(!std::is_default_constructible<HasOwnConstructor>::value, "own ctor removes default");

// Item 5: the compiler refuses to write operator= when it can't assign a const member.
static_assert(!std::is_copy_assignable<FilterPreset>::value, "const members block operator=");

// Item 6: the book's C++03 form. The base declares its copy functions private and never
// defines them. A derived class's generated copy functions can't call them, so its own
// members and friends get a compile error instead of a link error.
class Uncopyable {
protected:
    Uncopyable() = default;
    ~Uncopyable() = default;

private:
    Uncopyable(const Uncopyable&);
    Uncopyable& operator=(const Uncopyable&);
};

class UncopyableExample : private Uncopyable {};

static_assert(!std::is_copy_constructible<UncopyableExample>::value, "private base blocks copy");
static_assert(!std::is_copy_assignable<UncopyableExample>::value, "private base blocks assign");

// Item 6: the modern form.
static_assert(!std::is_copy_constructible<Logger>::value, "Logger is deleted");
static_assert(!std::is_copy_assignable<Logger>::value, "Logger is deleted");

namespace {

void demoBuiltInInitialization() {
    printHeading("Item 4: built-in types are not always initialized");
    std::cout << "new unsigned char[n] alone leaves its values indeterminate, and reading them\n"
              << "is undefined behaviour, so this demo never does. Image's constructor uses\n"
              << "new unsigned char[n]() instead, which zeroes every byte.\n";
    const Image image(8, 8, 3);
    bool allZero = true;
    for (std::size_t i = 0; i < image.getByteCount(); ++i) {
        allZero = allZero && image.getData()[i] == 0;
    }
    check(allZero, "a new Image has zeroed pixels");
}

void demoInitializationVersusAssignment() {
    printHeading("Item 4: initialize members instead of assigning them");
    std::cout << "A member not named in the initializer list is default-constructed first,\n"
              << "so assigning in the constructor body does the work twice.\n";
    compareInitializationCost();
}

void demoDeclarationOrder() {
    printHeading("Item 4: members initialize in declaration order");
    std::cout << "The order in the initializer list doesn't matter; the order in the class does.\n"
              << "Image allocates pixels from its dimensions, so pixels must be declared after\n"
              << "them. Listing them in a different order triggers -Wreorder.\n";
    const Image image(4, 2, 3);
    check(image.getByteCount() == 24, "pixels were sized from dimensions initialized before them");
}

void demoStartupOrder() {
    printHeading("Item 4: initialization order across source files");
    std::cout << "A global in one file that reads a global from another may run first. Reading\n"
              << "an int that early gives 0, while reading a class like std::string would be\n"
              << "undefined behaviour.\n";
    std::cout << "  global read at startup saw " << orderCopiedAtStartup << " (the real value is "
              << orderExpectedValue << "). This depends on the build's link order.\n";
    std::cout << "  function-local static read at startup saw " << orderLocalAtStartup << "\n";
    check(orderLocalAtStartup == orderExpectedValue,
          "a function-local static is ready whenever it is first used");
    check(&Logger::instance() == &Logger::instance(),
          "Logger uses the same fix, so any file can use it during startup");
}

void demoGeneratedFunctions() {
    printHeading("Item 5: what C++ silently writes and calls");
    std::cout << "PlainStruct declares nothing, yet can be default-constructed, copied, assigned\n"
              << "and destroyed. HasOwnConstructor loses its default constructor, and\n"
              << "FilterPreset's const members stop the compiler writing operator=. The\n"
              << "static_asserts in this file prove all of this at compile time.\n";
    PlainStruct first;
    first.id = 7;

    Probe::reset();
    PlainStruct second(first);
    check(second.id == 7 && Probe::copyConstructions == 1,
          "a generated copy constructor copies each member with its own copy constructor");

    PlainStruct third;
    Probe::reset();
    third = first; // NOLINT(performance-use-std-move): the copy is what this shows.
    check(third.id == 7 && Probe::copyAssignments == 1,
          "generated operator= assigns each member with its own operator=");

    std::cout << "Generated functions can also compile and still be wrong:\n";
    ShallowImage original(10);
    ShallowImage copy = original;
    check(original.data == copy.data, "ShallowImage's generated copy shares one buffer");
    copy.data = nullptr; // Stops both destructors from freeing the same buffer.
}

void demoDisallowingFunctions() {
    printHeading("Item 6: disallow generated functions you don't want");
    std::cout << "Only one Logger should exist, so its copy functions are marked = delete, and\n"
              << "any attempt to use them is a clear compile error.\n"
              << "Before C++11, a class declared them private and never defined them. That\n"
              << "stops outsiders, but a member or friend that copies fails only at link time.\n"
              << "An Uncopyable base moves that error to compile time, because the compiler\n"
              << "can't generate a copy for a class whose base can't be copied. = delete gives\n"
              << "a compile error everywhere, with a clear message. The static_asserts in this\n"
              << "file prove both forms at compile time.\n";
    check(&Logger::instance() == &Logger::instance(), "there is exactly one Logger");
}

} // namespace

void runLab4() {
    std::cout << "\n=== Lucrarea 4: Items 4, 5 and 6 ===\n";
    demoBuiltInInitialization();
    demoInitializationVersusAssignment();
    demoDeclarationOrder();
    demoStartupOrder();
    demoGeneratedFunctions();
    demoDisallowingFunctions();
}
