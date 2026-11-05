#ifndef IMAGE_IO_HPP
#define IMAGE_IO_HPP

#include "Image.hpp"
#include "LabeledImage.hpp"

#include <string>

// Reads any format stb_image supports, labelled with its path. Throws std::runtime_error.
LabeledImage loadImage(const std::string& path);

// Writes .png, .jpg, .jpeg or .bmp, chosen by extension in any letter case.
// Throws std::runtime_error.
void saveImage(const Image& image, const std::string& path);

#endif // IMAGE_IO_HPP
