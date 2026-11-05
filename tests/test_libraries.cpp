#include "stb_image.h"
#include "stb_image_write.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <vector>

TEST_CASE("stb writes and reads a PNG") {
    const char* path = "stb_round_trip.png";
    std::vector<unsigned char> pixels(4 * 4 * 3, 200);
    REQUIRE(stbi_write_png(path, 4, 4, 3, pixels.data(), 4 * 3) != 0);

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* loaded = stbi_load(path, &width, &height, &channels, 0);
    REQUIRE(loaded != nullptr);
    CHECK(width == 4);
    CHECK(height == 4);
    CHECK(channels == 3);
    CHECK(loaded[0] == 200);

    stbi_image_free(loaded);
    std::remove(path);
}
