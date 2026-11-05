#include "Demos.hpp"
#include "Image.hpp"
#include "LabeledImage.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
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

// Item 12: exposure was added after the copy functions were written, and the compiler
// says nothing about it.
struct ForgetsNewField {
    std::string name;
    int exposure = 0; // Added later.

    explicit ForgetsNewField(const std::string& newName) : name(newName) {
    }
    ForgetsNewField(const ForgetsNewField& other) : name(other.name) {
    }
    ForgetsNewField& operator=(const ForgetsNewField& other) {
        name = other.name;
        return *this;
    }
};

// Item 12: copies its own member and forgets the Image part.
class BrokenLabeledImage : public Image {
public:
    BrokenLabeledImage(const Image& image, const std::string& newLabel)
        : Image(image), label(newLabel) {
    }
    // Compiles, because Image has a default constructor, which silently runs here.
    // NOLINTNEXTLINE(bugprone-copy-constructor-init): this bug is what the demo shows.
    BrokenLabeledImage(const BrokenLabeledImage& other) : label(other.label) {
    }
    // Compiles, and leaves the Image part as it was.
    BrokenLabeledImage& operator=(const BrokenLabeledImage& other) {
        label = other.label;
        return *this;
    }

    const std::string& getLabel() const {
        return label;
    }

private:
    std::string label;
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
    std::cout << "  NaiveImage size after self-assignment: " << naive[i].getSize() << "\n"
              << "operator= freed the buffer and cleared the size first. The source is the same\n"
              << "object, so nothing is left to copy, and the image ends up empty.\n"
              << "A version which keeps the dimensions would read freed memory instead.\n";
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

void demoForgottenMember() {
    std::cout << "Part A: a member added after the copy functions were written.\n";
    ForgetsNewField original("shot");
    original.exposure = 400;

    const ForgetsNewField copied(original);
    check(copied.name == "shot" && copied.exposure == 0,
          "a hand-written copy constructor silently loses a newly added member");

    ForgetsNewField assigned("other");
    assigned = original;
    check(assigned.exposure == 0, "a hand-written operator= silently loses it as well");
}

void demoForgottenBase() {
    std::cout << "\nPart B: a derived class that forgets its base part.\n";
    Image pixels(2, 2, 1);
    pixels.set(0, 0, 0, 200);
    Image otherPixels(1, 1, 1);
    otherPixels.set(0, 0, 0, 7);

    const BrokenLabeledImage brokenSource(pixels, "source.png");
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization): the copy is the demo.
    const BrokenLabeledImage brokenCopy(brokenSource);
    check(brokenCopy.getLabel() == "source.png" && brokenCopy.isEmpty(),
          "a broken copy constructor keeps the label and silently produces no pixels");

    BrokenLabeledImage brokenTarget(otherPixels, "target.png");
    brokenTarget = brokenSource;
    check(brokenTarget.getLabel() == "source.png" && brokenTarget.getWidth() == 1 &&
              brokenTarget.at(0, 0, 0) == 7,
          "a broken operator= takes the new label and silently keeps the old pixels");

    const LabeledImage source(pixels, "source.png");
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization): the copy is the demo.
    const LabeledImage copy(source);
    check(copy.getLabel() == "source.png" && samePixels(copy, pixels),
          "LabeledImage's copy constructor copies the label and the pixels");

    LabeledImage target(otherPixels, "target.png");
    target = source;
    check(target.getLabel() == "source.png" && samePixels(target, pixels),
          "LabeledImage's operator= copies the label and the pixels");
}

void demoCopyingAllParts() {
    printHeading("Item 12: copy all parts of an object");
    std::cout << "Once you write copy functions yourself, the compiler no longer copies members\n"
              << "for you, and it says nothing when you miss a part.\n";
    demoForgottenMember();
    demoForgottenBase();
    std::cout << "\nPart C: sharing code between the two copy functions.\n"
              << "Their logic overlaps, but neither should call the other. Calling operator=\n"
              << "from a copy constructor assigns to an object that isn't constructed yet, and\n"
              << "calling a copy constructor from operator= tries to construct one that already\n"
              << "exists. Image puts the shared work in a private helper, cloneBuffer, instead.\n";
}

} // namespace

void runLab5() {
    std::cout << "\n=== Lucrarea 5: Items 10, 11 and 12 ===\n";
    demoReturningThis();
    demoSelfAssignment();
    demoCopyingAllParts();
}
