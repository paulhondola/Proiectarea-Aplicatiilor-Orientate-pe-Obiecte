#include "Filters.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {

constexpr int minBlurRadius = 1;
constexpr int maxBlurRadius = 64;

bool hasAlpha(const Image& image) {
    return image.getChannels() == 2 || image.getChannels() == 4;
}

int parseRadius(const std::string& text, const std::string& spec) {
    const auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
    if (text.empty() || !std::all_of(text.begin(), text.end(), isDigit)) {
        throw std::invalid_argument("Invalid blur radius in filter: " + spec);
    }
    try {
        return std::stoi(text);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Blur radius is too large in filter: " + spec);
    }
}

} // namespace

Image grayscale(const Image& image) {
    Image result(image);
    if (image.getChannels() < 3) {
        return result;
    }

    const auto stride = static_cast<std::size_t>(image.getChannels());
    unsigned char* pixels = result.getData();
    for (std::size_t i = 0; i < image.getByteCount(); i += stride) {
        const double luma = 0.299 * pixels[i] + 0.587 * pixels[i + 1] + 0.114 * pixels[i + 2];
        const auto gray = static_cast<unsigned char>(std::lround(luma));
        pixels[i] = gray;
        pixels[i + 1] = gray;
        pixels[i + 2] = gray;
    }
    return result;
}

Image invert(const Image& image) {
    Image result(image);
    const auto stride = static_cast<std::size_t>(image.getChannels());
    const std::size_t colourChannels = hasAlpha(image) ? stride - 1 : stride;
    unsigned char* pixels = result.getData();
    for (std::size_t i = 0; i < image.getByteCount(); i += stride) {
        for (std::size_t channel = 0; channel < colourChannels; ++channel) {
            pixels[i + channel] = static_cast<unsigned char>(255 - pixels[i + channel]);
        }
    }
    return result;
}

Image boxBlur(const Image& image, int radius) {
    if (radius < minBlurRadius || radius > maxBlurRadius) {
        throw std::invalid_argument("Blur radius must be between 1 and 64");
    }
    Image result(image);

    const int width = image.getWidth();
    const int height = image.getHeight();
    const int channels = image.getChannels();
    const unsigned char* source = image.getData();
    unsigned char* target = result.getData();

    // ponytail: two 1D passes instead of a 2D window, so cost grows with the radius and not
    // with its square. Sums aren't rounded until the end, so the result matches a 2D window.
    const auto offset = [&](int x, int y, int channel) {
        return (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(x)) *
                   static_cast<std::size_t>(channels) +
               static_cast<std::size_t>(channel);
    };

    std::vector<unsigned int> rowSums(image.getByteCount());
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int channel = 0; channel < channels; ++channel) {
                unsigned int sum = 0;
                for (int dx = -radius; dx <= radius; ++dx) {
                    sum += source[offset(std::clamp(x + dx, 0, width - 1), y, channel)];
                }
                rowSums[offset(x, y, channel)] = sum;
            }
        }
    }

    const unsigned int windowSide = 2U * static_cast<unsigned int>(radius) + 1U;
    const unsigned int windowSize = windowSide * windowSide;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int channel = 0; channel < channels; ++channel) {
                unsigned int sum = 0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    sum += rowSums[offset(x, std::clamp(y + dy, 0, height - 1), channel)];
                }
                target[offset(x, y, channel)] =
                    static_cast<unsigned char>((sum + windowSize / 2U) / windowSize);
            }
        }
    }
    return result;
}

Image applyFilterSpec(const Image& image, const std::string& spec) {
    if (spec == "grayscale") {
        return grayscale(image);
    }
    if (spec == "invert") {
        return invert(image);
    }
    const std::string blurPrefix = "blur:";
    if (spec.compare(0, blurPrefix.size(), blurPrefix) == 0) {
        return boxBlur(image, parseRadius(spec.substr(blurPrefix.size()), spec));
    }
    throw std::invalid_argument("Unknown filter: " + spec +
                                " (use grayscale, invert or blur:<radius>)");
}
