#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <cstddef>

// An image of 8-bit interleaved pixels. Owns one heap buffer through a raw
// pointer on purpose: the labs grade writing its special members by hand.
class Image {
public:
    Image();                                    // empty: 0x0x0, nullptr
    Image(int width, int height, int channels); // zero-filled
    ~Image();
    Image(const Image& other);            // deep copy
    Image(Image&& other) noexcept;        // takes the buffer; other becomes empty
    Image& operator=(const Image& other); // deep copy; safe for self-assignment

    int getWidth() const;
    int getHeight() const;
    int getChannels() const;
    bool isEmpty() const;
    std::size_t getByteCount() const;

    unsigned char at(int x, int y, int channel) const; // bounds-checked
    void set(int x, int y, int channel, unsigned char value);
    const unsigned char* getData() const;
    unsigned char* getData();

private:
    static unsigned char* cloneBuffer(const Image& source);
    std::size_t offsetOf(int x, int y, int channel) const; // throws std::out_of_range

    // Declaration order matters: pixels is initialized from the dimensions.
    int width;
    int height;
    int channels;
    unsigned char* pixels;
};

#endif // IMAGE_HPP
