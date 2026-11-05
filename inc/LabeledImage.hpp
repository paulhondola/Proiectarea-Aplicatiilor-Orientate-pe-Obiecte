#ifndef LABELED_IMAGE_HPP
#define LABELED_IMAGE_HPP

#include "Image.hpp"

#include <string>

// An image plus a label, such as the path it was loaded from.
//
// ~Image is not virtual, so a LabeledImage must never be deleted through an Image*.
class LabeledImage : public Image {
public:
    LabeledImage();
    LabeledImage(const Image& image, const std::string& newLabel);

    // Item 12: both must copy the base part as well as label.
    LabeledImage(const LabeledImage& other);
    LabeledImage& operator=(const LabeledImage& other);

    const std::string& getLabel() const;

private:
    std::string label;
};

#endif // LABELED_IMAGE_HPP
