#include "Demos.hpp"
#include "Image.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

bool samePixels(const Image& first, const Image& second) {
    return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() &&
           first.getChannels() == second.getChannels() &&
           std::equal(first.getData(), first.getData() + first.getByteCount(), second.getData());
}

// Item 10: compiles, but "a = b = c" doesn't, because b = c has no value to assign.
struct VoidAssignImage {
    void operator=(const VoidAssignImage&) {
    }
};
static_assert(std::is_void<decltype(std::declval<VoidAssignImage&>() =
                                        std::declval<const VoidAssignImage&>())>::value,
              "this operator= returns nothing, so it can't chain");
static_assert(
    std::is_same<decltype(std::declval<Image&>() = std::declval<const Image&>()), Image&>::value,
    "Image's operator= returns a reference to *this");

// Item 11: its operator= assumes the source is a different object.
class NaiveImage {
public:
    explicit NaiveImage(std::size_t newSize) : size(newSize), data(new unsigned char[newSize]()) {
    }
    NaiveImage(const NaiveImage& other) : size(other.size), data(new unsigned char[other.size]) {
        std::copy(other.data, other.data + other.size, data);
    }
    ~NaiveImage() {
        delete[] data;
    }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment): this bug is what the demo shows.
    NaiveImage& operator=(const NaiveImage& other) {
        // Releasing first looks tidy, but when other is *this it also destroys the source.
        delete[] data;
        data = nullptr;
        size = 0;

        // Clearing the size first keeps this bug well defined: nothing below reads freed
        // memory. A version that kept the old size would copy from the freed buffer.
        if (other.size > 0) {
            data = new unsigned char[other.size];
            std::copy(other.data, other.data + other.size, data);
            size = other.size;
        }
        return *this;
    }

    std::size_t getSize() const {
        return size;
    }

private:
    std::size_t size;
    unsigned char* data;
};

void demoReturningThis() {
    printHeading("Item 10: assignment returns a reference to *this");
    std::cout << "Built-in types let you write a = b = c, which runs right to left. Returning\n"
              << "*this gives user types the same behaviour. VoidAssignImage returns nothing,\n"
              << "so chaining it doesn't compile; a static_assert in this file proves its type.\n";
    Image c(2, 2, 1);
    c.set(1, 1, 0, 50);
    Image a(1, 1, 1);
    Image b(3, 3, 3);

    a = b = c;

    check(samePixels(a, c) && samePixels(b, c), "a = b = c leaves all three images equal");
    check(a.getData() != b.getData() && b.getData() != c.getData(),
          "each image in the chain keeps its own buffer");
}

void demoSelfAssignment() {
    printHeading("Item 11: handle assignment to self");
    std::cout << "Nobody writes x = x on purpose, but it happens through aliases, such as\n"
              << "images[i] = images[j] when i and j are equal.\n";
    const std::size_t i = 1;
    const std::size_t j = 1;

    std::vector<NaiveImage> naive(3, NaiveImage(16));
    naive[i] = naive[j];
    std::cout << "  NaiveImage size after self-assignment: " << naive[i].getSize() << "\n";
    check(naive[i].getSize() == 0, "self-assignment silently wipes a NaiveImage");

    std::vector<Image> images(3, Image(4, 4, 1));
    images[i].set(2, 2, 0, 123);
    const unsigned char* address = images[i].getData();
    images[i] = images[j];
    check(images[i].at(2, 2, 0) == 123, "self-assignment keeps an Image's pixels");
    check(images[i].getData() == address, "self-assignment keeps an Image's buffer");

    std::cout << "There are three common defences:\n"
              << "  1. An identity test, which is simple but alone doesn't help if new throws.\n"
              << "  2. Careful ordering: allocate and copy first, and only then free the old\n"
              << "     buffer. This is safe for self-assignment and for exceptions.\n"
              << "  3. Copy-and-swap, which builds a temporary copy and swaps with it.\n"
              << "Image uses 2, and adds 1 so that self-assignment skips copying every pixel.\n";
}

} // namespace

void runLab5() {
    std::cout << "\n=== Lucrarea 5: Items 10, 11 and 12 ===\n";
    demoReturningThis();
    demoSelfAssignment();
}
