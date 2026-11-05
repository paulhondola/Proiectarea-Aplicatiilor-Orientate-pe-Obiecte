#include "Image.hpp"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <type_traits>
#include <utility>

static_assert(std::is_nothrow_move_constructible<Image>::value, "moves must not throw");

namespace {

// A 2x2 single-channel image holding 1, 2, 3, 4.
Image makeNumbered() {
    Image image(2, 2, 1);
    image.set(0, 0, 0, 1);
    image.set(1, 0, 0, 2);
    image.set(0, 1, 0, 3);
    image.set(1, 1, 0, 4);
    return image;
}

bool isNumbered(const Image& image) {
    return image.getWidth() == 2 && image.getHeight() == 2 && image.getChannels() == 1 &&
           image.at(0, 0, 0) == 1 && image.at(1, 0, 0) == 2 && image.at(0, 1, 0) == 3 &&
           image.at(1, 1, 0) == 4;
}

} // namespace

TEST_CASE("default image is empty") {
    const Image image;
    CHECK(image.getWidth() == 0);
    CHECK(image.getHeight() == 0);
    CHECK(image.getChannels() == 0);
    CHECK(image.getByteCount() == 0);
    CHECK(image.isEmpty());
    CHECK(image.getData() == nullptr);
}

TEST_CASE("new image is zero-filled") {
    const Image image(2, 3, 3);
    CHECK_FALSE(image.isEmpty());
    REQUIRE(image.getByteCount() == 18);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 2; ++x) {
            for (int c = 0; c < 3; ++c) {
                CHECK(image.at(x, y, c) == 0);
            }
        }
    }
}

TEST_CASE("invalid dimensions throw") {
    CHECK_THROWS_AS(Image(0, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(Image(1, 0, 1), std::invalid_argument);
    CHECK_THROWS_AS(Image(-1, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(Image(1, -1, 1), std::invalid_argument);
    CHECK_THROWS_AS(Image(1, 1, 0), std::invalid_argument);
    CHECK_THROWS_AS(Image(1, 1, 5), std::invalid_argument);
}

TEST_CASE("at and set reject bad coordinates") {
    Image image(2, 2, 1);
    const int bad[][3] = {{-1, 0, 0}, {2, 0, 0}, {0, -1, 0}, {0, 2, 0}, {0, 0, -1}, {0, 0, 1}};
    for (const auto& coordinate : bad) {
        CHECK_THROWS_AS(image.at(coordinate[0], coordinate[1], coordinate[2]), std::out_of_range);
        CHECK_THROWS_AS(image.set(coordinate[0], coordinate[1], coordinate[2], 9),
                        std::out_of_range);
    }
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            CHECK(image.at(x, y, 0) == 0);
        }
    }
}

TEST_CASE("empty image rejects every coordinate") {
    const Image image;
    CHECK_THROWS_AS(image.at(0, 0, 0), std::out_of_range);
}

TEST_CASE("copy is deep") {
    const Image original = makeNumbered();
    Image copy(original);
    CHECK(copy.getData() != original.getData());
    CHECK(isNumbered(copy));

    copy.set(0, 0, 0, 99);
    CHECK(copy.at(0, 0, 0) == 99);
    CHECK(isNumbered(original));
}

TEST_CASE("copy of empty is empty") {
    const Image original;
    const Image copy(original); // NOLINT(performance-unnecessary-copy-initialization): under test
    CHECK(copy.isEmpty());
    CHECK(copy.getData() == nullptr);
}

TEST_CASE("move takes the buffer") {
    Image source = makeNumbered();
    const unsigned char* address = source.getData();

    const Image target(std::move(source));

    CHECK(target.getData() == address);
    CHECK(isNumbered(target));
    // Reading a moved-from Image is safe by design: this is what the test proves.
    // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    CHECK(source.isEmpty());
    CHECK(source.getData() == nullptr);
    CHECK(source.getWidth() == 0);
    CHECK(source.getByteCount() == 0);
    // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

static_assert(
    std::is_same<decltype(std::declval<Image&>() = std::declval<const Image&>()), Image&>::value,
    "operator= must return Image&");

TEST_CASE("assignment deep-copies across sizes") {
    Image source(3, 2, 4);
    source.set(2, 1, 3, 77);
    Image target(1, 1, 1);

    target = source;

    CHECK(target.getWidth() == 3);
    CHECK(target.getHeight() == 2);
    CHECK(target.getChannels() == 4);
    CHECK(target.at(2, 1, 3) == 77);
    CHECK(target.getData() != source.getData());

    source.set(2, 1, 3, 1);
    CHECK(target.at(2, 1, 3) == 77);
}

TEST_CASE("assignment chains") {
    const Image c = makeNumbered();
    Image a(1, 1, 1);
    Image b(1, 1, 1);

    a = b = c;

    CHECK(isNumbered(a));
    CHECK(isNumbered(b));
    CHECK(a.getData() != b.getData());
    CHECK(b.getData() != c.getData());
}

TEST_CASE("self-assignment keeps pixels and address") {
    Image image = makeNumbered();
    const unsigned char* address = image.getData();
    const Image& alias = image;

    image = alias;

    CHECK(isNumbered(image));
    CHECK(image.getData() == address);
}

TEST_CASE("assigning from empty empties the target") {
    Image target = makeNumbered();
    const Image empty;

    target = empty;

    CHECK(target.isEmpty());
    CHECK(target.getData() == nullptr);
    CHECK(target.getByteCount() == 0);
}

TEST_CASE("assigning to an empty target fills it") {
    Image target;
    const Image source = makeNumbered();

    target = source;

    CHECK(isNumbered(target));
}

TEST_CASE("a moved-from image can be assigned to again") {
    Image source = makeNumbered();
    const Image moved(std::move(source));

    source = moved; // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move): under test

    CHECK(isNumbered(source));
    CHECK(source.getData() != moved.getData());
}
