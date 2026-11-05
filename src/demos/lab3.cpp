#include "DemoTypes.hpp"
#include "Demos.hpp"
#include "Image.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

template <typename Exception, typename Action> bool throws(Action action) {
    try {
        action();
    } catch (const Exception&) {
        return true;
    }
    return false;
}

// Two constructors with the same result, reached in different ways.
struct AssignsInBody {
    Probe probe;
    explicit AssignsInBody(const Probe& source) {
        probe = source;
    }
};

struct UsesInitList {
    Probe probe;
    explicit UsesInitList(const Probe& source) : probe(source) {
    }
};

// Counts the buffers still allocated, with and without delete[] in a destructor.
int allocatedBuffers = 0;

unsigned char* allocateBuffer() {
    ++allocatedBuffers;
    return new unsigned char[256]();
}

void freeBuffer(unsigned char* buffer) {
    delete[] buffer;
    --allocatedBuffers;
}

struct LeakyBuffer {
    unsigned char* data = allocateBuffer();
};

struct OwningBuffer {
    unsigned char* data = allocateBuffer();
    ~OwningBuffer() {
        freeBuffer(data);
    }
};

void demoEncapsulation() {
    printHeading("Encapsulation");
    std::cout << "Pixels are private, so every access goes through a bounds check.\n";
    Image image(2, 2, 3);
    image.set(1, 1, 2, 200);

    check(throws<std::out_of_range>([&] { image.at(2, 0, 0); }), "at rejects x past the width");
    check(throws<std::out_of_range>([&] { image.set(0, 0, 3, 9); }),
          "set rejects a channel past the last one");
    check(throws<std::invalid_argument>([] { Image(0, 4, 3); }),
          "the constructor rejects a zero width");
    check(image.at(1, 1, 2) == 200 && image.at(0, 0, 0) == 0,
          "rejected calls leave the pixels unchanged");
}

void demoConstructorInitialization() {
    printHeading("Initializing members in the constructor");
    std::cout << "Image's initializer list sets the dimensions, then allocates pixels from them.\n"
              << "This relies on pixels being declared after the dimensions, because members\n"
              << "initialize in declaration order.\n"
              << "const members leave no choice: assigning to one in the constructor body does\n"
              << "not compile, so FilterPreset must use the initializer list.\n"
              << "Other members allow assignment in the body, but it costs more, because the\n"
              << "member is default-constructed first:\n";
    compareInitializationCost();

    const Image image(3, 2, 4);
    check(image.getByteCount() == 24, "pixels were allocated from the initialized dimensions");

    const FilterPreset preset("soft", 2);
    check(preset.name == "soft" && preset.radius == 2, "FilterPreset keeps its const members");
}

void demoDestructor() {
    printHeading("Freeing the heap in the destructor");
    std::cout << "A class that allocates in its constructor has to free in its destructor, or\n"
              << "every object leaves its buffer behind.\n";

    unsigned char* leaked = nullptr;
    {
        const LeakyBuffer scoped;
        leaked = scoped.data;
    }
    std::cout << "  LeakyBuffer has no delete[]: " << allocatedBuffers
              << " buffer still allocated after the scope\n";
    check(allocatedBuffers == 1, "without delete[] the buffer stays allocated after the scope");
    freeBuffer(leaked); // Only this demo kept the pointer. A real leak has nothing left to free.

    {
        const OwningBuffer scoped;
    }
    std::cout << "  OwningBuffer has delete[]: " << allocatedBuffers
              << " buffers still allocated after the scope\n";
    check(allocatedBuffers == 0, "with delete[] no buffer is left allocated");

    std::cout << "~Image frees its pixels the same way. Entering a scope:\n";

    std::ostringstream captured;
    std::streambuf* previous = std::cout.rdbuf(captured.rdbuf());
    {
        const Image scoped(16, 16, 3);
    }
    std::cout.rdbuf(previous);

    std::cout << captured.str() << "Left the scope.\n";
    check(captured.str().find("Image dtor") != std::string::npos,
          "leaving the scope ran Image's destructor");
}

void demoCopyConstructor() {
    printHeading("Writing the copy constructor");
    std::cout << "Part A: ShallowImage uses the copy constructor the compiler generates.\n";
    {
        ShallowImage original(10);
        ShallowImage copy = original;
        std::cout << "  original buffer " << static_cast<const void*>(original.data) << "\n"
                  << "  copy buffer     " << static_cast<const void*>(copy.data) << "\n";
        check(original.data == copy.data, "a generated copy shares the buffer");

        copy.data[0] = 99;
        check(original.data[0] == 99, "a write through the copy changes the original");

        std::cout << "Clearing one pointer here, because both destructors would otherwise free\n"
                  << "the same buffer, which is a double free.\n";
        copy.data = nullptr;
    }

    std::cout << "\nPart B: Image writes its own copy constructor.\n";
    Image original(2, 2, 1);
    original.set(0, 0, 0, 10);
    Image copy(original);
    check(copy.getData() != original.getData(), "an Image copy has its own buffer");
    check(copy.at(0, 0, 0) == 10, "an Image copy has the same pixels");

    copy.set(0, 0, 0, 99);
    check(original.at(0, 0, 0) == 10, "a write through the copy leaves the original unchanged");
}

long long microsecondsSince(std::chrono::steady_clock::time_point start) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

void demoMoveConstructor() {
    printHeading("Creating a move constructor");
    std::cout << "Copying has to duplicate every pixel, while moving only hands over a pointer.\n"
              << "Comparing both on a 4096x4096 RGB image:\n";
    Image source(4096, 4096, 3);
    const unsigned char* address = source.getData();

    auto start = std::chrono::steady_clock::now();
    const Image copied(source);
    const long long copyTime = microsecondsSince(start);

    start = std::chrono::steady_clock::now();
    const Image moved(std::move(source));
    const long long moveTime = microsecondsSince(start);

    std::cout << "  copy took " << copyTime << " us, move took " << moveTime << " us\n";
    check(copied.getData() != address, "the copy allocated a new buffer");
    check(moved.getData() == address, "the move kept the same buffer");
    // Reading a moved-from Image is safe by design: its move constructor leaves it empty.
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    check(source.isEmpty() && source.getData() == nullptr, "the move emptied the source");
}

} // namespace

void compareInitializationCost() {
    const Probe source;

    Probe::reset();
    const AssignsInBody assigned(source);
    std::cout << "  assignment in the body: " << Probe::defaultConstructions << " default, "
              << Probe::copyAssignments << " assignment, " << Probe::copyConstructions << " copy\n";
    check(Probe::defaultConstructions == 1 && Probe::copyAssignments == 1 &&
              Probe::copyConstructions == 0,
          "assigning in the body costs a default construction and an assignment");

    Probe::reset();
    const UsesInitList listed(source);
    std::cout << "  initializer list:       " << Probe::defaultConstructions << " default, "
              << Probe::copyAssignments << " assignment, " << Probe::copyConstructions << " copy\n";
    check(Probe::defaultConstructions == 0 && Probe::copyAssignments == 0 &&
              Probe::copyConstructions == 1,
          "an initializer list costs one copy construction");
}

void runLab3() {
    std::cout << "\n=== Lucrarea 3: object lifetime ===\n";
    demoEncapsulation();
    demoConstructorInitialization();
    demoDestructor();
    demoCopyConstructor();
    demoMoveConstructor();
}
