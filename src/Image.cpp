#include "Image.hpp"

#include "Logger.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

std::size_t checkedByteCount(int width, int height, int channels) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Image width and height must be positive");
    }
    if (channels < 1 || channels > 4) {
        throw std::invalid_argument("Image channels must be between 1 and 4");
    }
    const std::size_t limit = std::numeric_limits<std::size_t>::max();
    const std::size_t rowBytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    if (static_cast<std::size_t>(height) > limit / rowBytes) {
        throw std::invalid_argument("Image is too large to address");
    }
    return rowBytes * static_cast<std::size_t>(height);
}

// Never throws, because the destructor and the move constructor rely on it.
// The action is a plain pointer so that calling this builds no temporary string.
void traceBuffer(const char* action, const void* buffer) noexcept {
    try {
        std::ostringstream message;
        message << "[" << action << "] buffer " << buffer;
        Logger::instance().trace(message.str());
        // NOLINTNEXTLINE(bugprone-empty-catch): a failed log line must not end the program.
    } catch (const std::exception&) {
    }
}

} // namespace

Image::Image() : width(0), height(0), channels(0), pixels(nullptr) {
    traceBuffer("Image default ctor", pixels);
}

Image::Image(int newWidth, int newHeight, int newChannels)
    : width(newWidth), height(newHeight), channels(newChannels),
      pixels(new unsigned char[checkedByteCount(width, height, channels)]()) {
    traceBuffer("Image ctor", pixels);
}

Image::~Image() {
    traceBuffer("Image dtor", pixels);
    delete[] pixels;
}

Image::Image(const Image& other)
    : width(other.width), height(other.height), channels(other.channels),
      pixels(cloneBuffer(other)) {
    traceBuffer("Image copy ctor", pixels);
}

Image::Image(Image&& other) noexcept
    : width(other.width), height(other.height), channels(other.channels), pixels(other.pixels) {
    other.width = 0;
    other.height = 0;
    other.channels = 0;
    other.pixels = nullptr;
    traceBuffer("Image move ctor", pixels);
}

int Image::getWidth() const {
    return width;
}

int Image::getHeight() const {
    return height;
}

int Image::getChannels() const {
    return channels;
}

bool Image::isEmpty() const {
    return pixels == nullptr;
}

std::size_t Image::getByteCount() const {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
           static_cast<std::size_t>(channels);
}

unsigned char Image::at(int x, int y, int channel) const {
    return pixels[offsetOf(x, y, channel)];
}

void Image::set(int x, int y, int channel, unsigned char value) {
    pixels[offsetOf(x, y, channel)] = value;
}

const unsigned char* Image::getData() const {
    return pixels;
}

unsigned char* Image::getData() {
    return pixels;
}

unsigned char* Image::cloneBuffer(const Image& source) {
    if (source.pixels == nullptr) {
        return nullptr;
    }
    const std::size_t byteCount = source.getByteCount();
    unsigned char* buffer = new unsigned char[byteCount];
    std::copy(source.pixels, source.pixels + byteCount, buffer);
    return buffer;
}

std::size_t Image::offsetOf(int x, int y, int channel) const {
    if (x < 0 || x >= width || y < 0 || y >= height || channel < 0 || channel >= channels) {
        throw std::out_of_range("Image coordinate is out of range");
    }
    const std::size_t rowBytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    return static_cast<std::size_t>(y) * rowBytes +
           static_cast<std::size_t>(x) * static_cast<std::size_t>(channels) +
           static_cast<std::size_t>(channel);
}
