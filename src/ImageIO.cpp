#include "ImageIO.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace {

constexpr int jpegQuality = 90;
constexpr int jpegMaxDimension = 65535; // JPEG stores each dimension in 16 bits.

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

// Receives stb's output. stb's own file functions ignore write errors, such as a full disk.
void writeToStream(void* stream, void* data, int size) {
    static_cast<std::ofstream*>(stream)->write(static_cast<const char*>(data), size);
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

    const bool isPng = extension == ".png";
    const bool isJpeg = extension == ".jpg" || extension == ".jpeg";
    if (!isPng && !isJpeg && extension != ".bmp") {
        throw std::runtime_error("Cannot write " + path + ": use .png, .jpg, .jpeg or .bmp");
    }
    if (isJpeg && (width > jpegMaxDimension || height > jpegMaxDimension)) {
        throw std::runtime_error("Cannot write " + path + ": JPEG allows at most " +
                                 std::to_string(jpegMaxDimension) + " pixels per side");
    }

    // Written beside the target and renamed over it only once complete, so a failed write
    // can't damage an existing file, which may even be the input.
    // ponytail: fixed name, so an existing <path>.tmp is overwritten; use a unique name if
    // several runs ever write the same output at once.
    const std::string temporaryPath = path + ".tmp";
    std::ofstream file(temporaryPath, std::ios::binary);
    int written = 0;
    if (isPng) {
        written = stbi_write_png_to_func(writeToStream, &file, width, height, channels, pixels,
                                         width * channels);
    } else if (isJpeg) {
        written = stbi_write_jpg_to_func(writeToStream, &file, width, height, channels, pixels,
                                         jpegQuality);
    } else {
        written = stbi_write_bmp_to_func(writeToStream, &file, width, height, channels, pixels);
    }
    file.close(); // Flushes the buffer, so this is where a small write really fails.

    std::error_code error;
    if (written != 0 && !file.fail()) {
        std::filesystem::rename(temporaryPath, path, error);
        if (!error) {
            return;
        }
    }
    std::filesystem::remove(temporaryPath, error);
    throw std::runtime_error("Cannot write " + path);
}
