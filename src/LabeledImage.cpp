#include "LabeledImage.hpp"

LabeledImage::LabeledImage() = default;

LabeledImage::LabeledImage(const Image& image, const std::string& newLabel)
    : Image(image), label(newLabel) {
}

LabeledImage::LabeledImage(const LabeledImage& other) : Image(other), label(other.label) {
}

LabeledImage& LabeledImage::operator=(const LabeledImage& other) {
    Image::operator=(other);
    label = other.label;
    return *this;
}

const std::string& LabeledImage::getLabel() const {
    return label;
}
