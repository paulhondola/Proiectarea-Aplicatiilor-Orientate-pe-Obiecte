#include "ImageIO.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>

namespace {

constexpr int jpegQuality = 90;

std::string lowerCaseExtension(const std::string& path) {
    const std::size_t dot = path.rfind('.');
    if (dot == std::string::npos) {
        return "";
    }
    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}

} // namespace

LabeledImage loadImage(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    // The unique_ptr frees stb's malloc-based buffer even if allocating the Image throws.
    const std::unique_ptr<unsigned char, decltype(&stbi_image_free)> loaded(
        stbi_load(path.c_str(), &width, &height, &channels, 0), &stbi_image_free);
    if (!loaded) {
        throw std::runtime_error("Cannot read " + path + ": " + stbi_failure_reason());
    }

    Image image(width, height, channels);
    std::copy(loaded.get(), loaded.get() + image.getByteCount(), image.getData());
    return LabeledImage(image, path);
}

void saveImage(const Image& image, const std::string& path) {
    if (image.isEmpty()) {
        throw std::runtime_error("Cannot write " + path + ": the image is empty");
    }

    const int width = image.getWidth();
    const int height = image.getHeight();
    const int channels = image.getChannels();
    const unsigned char* pixels = image.getData();
    const std::string extension = lowerCaseExtension(path);

    int written = 0;
    if (extension == ".png") {
        written = stbi_write_png(path.c_str(), width, height, channels, pixels, width * channels);
    } else if (extension == ".jpg" || extension == ".jpeg") {
        written = stbi_write_jpg(path.c_str(), width, height, channels, pixels, jpegQuality);
    } else if (extension == ".bmp") {
        written = stbi_write_bmp(path.c_str(), width, height, channels, pixels);
    } else {
        throw std::runtime_error("Cannot write " + path + ": use .png, .jpg, .jpeg or .bmp");
    }
    if (written == 0) {
        throw std::runtime_error("Cannot write " + path);
    }
}
