#include "Filters.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace {

Image makeImage(int width, int height, int channels, std::initializer_list<int> values) {
    Image image(width, height, channels);
    std::transform(values.begin(), values.end(), image.getData(),
                   [](int value) { return static_cast<unsigned char>(value); });
    return image;
}

bool same(const Image& first, const Image& second) {
    return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() &&
           first.getChannels() == second.getChannels() &&
           std::equal(first.getData(), first.getData() + first.getByteCount(), second.getData());
}

} // namespace

TEST_CASE("grayscale uses rounded luma and keeps alpha") {
    // 0.299 * 255 = 76.245
    CHECK(
        same(grayscale(makeImage(1, 1, 4, {255, 0, 0, 77})), makeImage(1, 1, 4, {76, 76, 76, 77})));
    // 0.587 * 255 = 149.685
    CHECK(same(grayscale(makeImage(1, 1, 3, {0, 255, 0})), makeImage(1, 1, 3, {150, 150, 150})));
    // 0.114 * 255 = 29.07
    CHECK(same(grayscale(makeImage(1, 1, 3, {0, 0, 255})), makeImage(1, 1, 3, {29, 29, 29})));
    CHECK(
        same(grayscale(makeImage(1, 1, 3, {255, 255, 255})), makeImage(1, 1, 3, {255, 255, 255})));
}

TEST_CASE("grayscale copies 1- and 2-channel input") {
    const Image gray = makeImage(2, 1, 1, {10, 20});
    CHECK(same(grayscale(gray), gray));
    const Image grayAlpha = makeImage(1, 1, 2, {10, 77});
    CHECK(same(grayscale(grayAlpha), grayAlpha));
}

TEST_CASE("invert keeps alpha") {
    CHECK(same(invert(makeImage(1, 1, 3, {0, 10, 255})), makeImage(1, 1, 3, {255, 245, 0})));
    CHECK(
        same(invert(makeImage(1, 1, 4, {0, 10, 255, 77})), makeImage(1, 1, 4, {255, 245, 0, 77})));
    CHECK(same(invert(makeImage(1, 1, 2, {10, 77})), makeImage(1, 1, 2, {245, 77})));
    CHECK(same(invert(makeImage(1, 1, 1, {10})), makeImage(1, 1, 1, {245})));
}

TEST_CASE("box blur averages with clamped edges") {
    // x=0 averages (0,0,0), x=1 averages (0,0,90), x=2 averages (0,90,90).
    CHECK(same(boxBlur(makeImage(3, 1, 1, {0, 0, 90}), 1), makeImage(3, 1, 1, {0, 30, 60})));
}

TEST_CASE("box blur covers both directions") {
    // Every pixel's clamped 3x3 window holds the bright centre exactly once.
    CHECK(same(boxBlur(makeImage(3, 3, 1, {0, 0, 0, 0, 90, 0, 0, 0, 0}), 1),
               makeImage(3, 3, 1, {10, 10, 10, 10, 10, 10, 10, 10, 10})));
}

TEST_CASE("box blur rounds to nearest") {
    // (0,0,2)/3 = 0.67 rounds to 1, and (0,2,2)/3 = 1.33 rounds to 1.
    CHECK(same(boxBlur(makeImage(3, 1, 1, {0, 0, 2}), 1), makeImage(3, 1, 1, {0, 1, 1})));
}

TEST_CASE("box blur treats channels separately") {
    CHECK(same(boxBlur(makeImage(2, 1, 2, {0, 200, 30, 200}), 1),
               makeImage(2, 1, 2, {10, 200, 20, 200})));
}

TEST_CASE("blur on 1x1 returns the same pixel") {
    const Image pixel = makeImage(1, 1, 1, {42});
    CHECK(same(boxBlur(pixel, 64), pixel));
}

TEST_CASE("blur rejects radii outside 1 to 64") {
    const Image pixel = makeImage(1, 1, 1, {42});
    CHECK_THROWS_AS(boxBlur(pixel, 0), std::invalid_argument);
    CHECK_THROWS_AS(boxBlur(pixel, 65), std::invalid_argument);
    CHECK_THROWS_AS(boxBlur(pixel, -1), std::invalid_argument);
}

TEST_CASE("filters leave the input unchanged and return its dimensions") {
    const Image input = makeImage(2, 1, 3, {1, 2, 3, 4, 5, 6});
    const Image before(input); // NOLINT(performance-unnecessary-copy-initialization): a snapshot

    const Image results[] = {grayscale(input), invert(input), boxBlur(input, 1)};

    CHECK(same(input, before));
    for (const Image& result : results) {
        CHECK(result.getWidth() == 2);
        CHECK(result.getHeight() == 1);
        CHECK(result.getChannels() == 3);
    }
}

TEST_CASE("filters accept an empty image") {
    const Image empty;
    CHECK(grayscale(empty).isEmpty());
    CHECK(invert(empty).isEmpty());
    CHECK(boxBlur(empty, 1).isEmpty());
}

TEST_CASE("specs select filters") {
    const Image input = makeImage(2, 2, 3, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120});
    CHECK(same(applyFilterSpec(input, "grayscale"), grayscale(input)));
    CHECK(same(applyFilterSpec(input, "invert"), invert(input)));
    CHECK(same(applyFilterSpec(input, "blur:2"), boxBlur(input, 2)));
    CHECK(same(applyFilterSpec(input, "blur:64"), boxBlur(input, 64)));
}

TEST_CASE("bad specs are rejected") {
    const Image input = makeImage(1, 1, 1, {1});
    const char* bad[] = {
        "",         "sharpen",  "blur",    "blur:",   "blur:abc", "blur:2x",          "blur:0",
        "blur:65",  "blur:-1",  "blur:+2", "blur: 2", "blur:2:3", "blur:99999999999", "grayscale:3",
        "invert:1", "GRAYSCALE"};
    for (const char* spec : bad) {
        INFO("spec: " << spec);
        CHECK_THROWS_AS(applyFilterSpec(input, spec), std::invalid_argument);
    }
}
