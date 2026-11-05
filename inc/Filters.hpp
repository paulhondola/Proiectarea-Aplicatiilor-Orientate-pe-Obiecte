#ifndef FILTERS_HPP
#define FILTERS_HPP

#include "Image.hpp"

#include <string>

// Each filter returns a new image, with the same dimensions as its input.

// Sets colour channels to luma, and copies 1- and 2-channel images unchanged.
Image grayscale(const Image& image);

// Replaces each colour channel v with 255 - v. Alpha is kept.
Image invert(const Image& image);

// Box blur over all channels with clamped edges. Throws std::invalid_argument
// unless radius is between 1 and 64.
Image boxBlur(const Image& image, int radius);

// Applies "grayscale", "invert" or "blur:<radius>". Throws std::invalid_argument
// for anything else.
Image applyFilterSpec(const Image& image, const std::string& spec);

#endif // FILTERS_HPP
