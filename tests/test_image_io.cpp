#include "ImageIO.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

// Removes its file when the test ends, including when an assertion fails.
class TemporaryFile {
public:
    explicit TemporaryFile(const std::string& newPath) : path(newPath) {
    }
    ~TemporaryFile() {
        std::remove(path.c_str());
    }
    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;

    const std::string path;
};

Image makePattern(int width, int height, int channels) {
    Image image(width, height, channels);
    unsigned char value = 3;
    std::generate(image.getData(), image.getData() + image.getByteCount(), [&value] {
        value = static_cast<unsigned char>(value + 7);
        return value;
    });
    return image;
}

} // namespace

TEST_CASE("PNG round trip is lossless") {
    const TemporaryFile file("io_round_trip.png");
    const Image image = makePattern(3, 2, 4);

    saveImage(image, file.path);
    const LabeledImage loaded = loadImage(file.path);

    CHECK(loaded.getWidth() == 3);
    CHECK(loaded.getHeight() == 2);
    REQUIRE(loaded.getChannels() == 4);
    CHECK(std::equal(image.getData(), image.getData() + image.getByteCount(), loaded.getData()));
    CHECK(loaded.getLabel() == file.path);
}

TEST_CASE("PNG keeps each channel count") {
    for (int channels = 1; channels <= 4; ++channels) {
        const TemporaryFile file("io_channels.png");
        saveImage(makePattern(2, 2, channels), file.path);
        CHECK(loadImage(file.path).getChannels() == channels);
    }
}

TEST_CASE("uppercase extensions work") {
    const TemporaryFile file("io_upper.PNG");
    saveImage(makePattern(2, 2, 3), file.path);
    CHECK(loadImage(file.path).getWidth() == 2);
}

TEST_CASE("RGBA saved as JPEG loads with 3 channels") {
    const TemporaryFile file("io_rgba.jpg");
    saveImage(makePattern(4, 4, 4), file.path);
    const LabeledImage loaded = loadImage(file.path);
    CHECK(loaded.getChannels() == 3);
    CHECK(loaded.getWidth() == 4);
    CHECK(loaded.getHeight() == 4);
}

TEST_CASE("jpeg and bmp extensions save") {
    const TemporaryFile jpeg("io_format.jpeg");
    saveImage(makePattern(4, 4, 3), jpeg.path);
    CHECK(loadImage(jpeg.path).getWidth() == 4);

    const TemporaryFile bmp("io_format.bmp");
    saveImage(makePattern(4, 4, 3), bmp.path);
    CHECK(loadImage(bmp.path).getWidth() == 4);
}

TEST_CASE("load errors throw runtime_error") {
    CHECK_THROWS_AS(loadImage("io_does_not_exist.png"), std::runtime_error);

    const TemporaryFile notAnImage("io_not_an_image.png");
    std::ofstream(notAnImage.path) << "this is text";
    CHECK_THROWS_AS(loadImage(notAnImage.path), std::runtime_error);
}

TEST_CASE("save errors throw runtime_error") {
    const Image image = makePattern(2, 2, 3);
    CHECK_THROWS_AS(saveImage(image, "io_unsupported.gif"), std::runtime_error);
    CHECK_THROWS_AS(saveImage(image, "io_no_extension"), std::runtime_error);
    CHECK_THROWS_AS(saveImage(image, "io_missing_directory/out.png"), std::runtime_error);
    CHECK_THROWS_AS(saveImage(Image(), "io_empty.png"), std::runtime_error);
}

TEST_CASE("JPEG rejects dimensions its format can't store") {
    const TemporaryFile wide("io_too_wide.jpg");
    const TemporaryFile tall("io_too_tall.jpeg");
    CHECK_THROWS_AS(saveImage(Image(70000, 1, 1), wide.path), std::runtime_error);
    CHECK_THROWS_AS(saveImage(Image(1, 70000, 1), tall.path), std::runtime_error);
    CHECK_FALSE(std::ifstream(wide.path).good());
}

TEST_CASE("JPEG accepts the largest dimension it can store") {
    const TemporaryFile file("io_widest.jpg");
    saveImage(Image(65535, 1, 1), file.path);
    CHECK(loadImage(file.path).getWidth() == 65535);
}

TEST_CASE("a successful save leaves no temporary file") {
    const TemporaryFile file("io_clean.png");
    saveImage(makePattern(2, 2, 3), file.path);
    CHECK(std::ifstream(file.path).good());
    CHECK_FALSE(std::ifstream(file.path + ".tmp").good());
}
