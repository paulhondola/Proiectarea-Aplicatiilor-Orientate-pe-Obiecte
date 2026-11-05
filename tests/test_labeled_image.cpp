#include "LabeledImage.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

Image makeImage(int width, int height, int channels, unsigned char first) {
    Image image(width, height, channels);
    image.set(0, 0, 0, first);
    return image;
}

} // namespace

TEST_CASE("constructor keeps pixels and label") {
    const LabeledImage image(makeImage(2, 2, 1, 5), "photo.png");
    CHECK(image.at(0, 0, 0) == 5);
    CHECK(image.getLabel() == "photo.png");
}

TEST_CASE("copy keeps pixels and label") {
    const LabeledImage original(makeImage(2, 2, 1, 5), "photo.png");
    LabeledImage copy(original);

    CHECK(copy.getWidth() == 2);
    CHECK(copy.at(0, 0, 0) == 5);
    CHECK(copy.getLabel() == "photo.png");
    CHECK(copy.getData() != original.getData());
}

TEST_CASE("assignment replaces pixels and label") {
    const LabeledImage source(makeImage(3, 2, 4, 9), "source.png");
    LabeledImage target(makeImage(1, 1, 1, 1), "target.png");

    target = source;

    CHECK(target.getWidth() == 3);
    CHECK(target.getHeight() == 2);
    CHECK(target.getChannels() == 4);
    CHECK(target.at(0, 0, 0) == 9);
    CHECK(target.getLabel() == "source.png");
    CHECK(target.getData() != source.getData());
}

TEST_CASE("self-assignment keeps pixels and label") {
    LabeledImage image(makeImage(2, 2, 1, 5), "photo.png");
    const LabeledImage& alias = image;

    image = alias;

    CHECK(image.at(0, 0, 0) == 5);
    CHECK(image.getLabel() == "photo.png");
}

TEST_CASE("default LabeledImage is empty with an empty label") {
    const LabeledImage image;
    CHECK(image.isEmpty());
    CHECK(image.getLabel().empty());
}
