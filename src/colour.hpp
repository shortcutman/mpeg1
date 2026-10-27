
//------------------------------------------------------------------------------
// colour.hpp
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <ostream>
#include <span>
#include <vector>

namespace image {

struct Colour {
    union {
        int r = 0;
        int y;
    };
    
    union {
        int g = 0;
        int cb;
    };
    
    union {
        int b = 0;
        int cr;
    };
    
    bool operator==(const Colour& b) const {
        return this->r == b.r &&
               this->g == b.g &&
               this->b == b.b;
    }

    Colour operator/(const int& divisor) const {
        return Colour{
            .r = this->r / divisor,
            .g = this->g / divisor,
            .b = this->b / divisor
        };
    }

    Colour operator+(const Colour& other) const {
        return Colour{
            .r = this->r + other.r,
            .g = this->g + other.g,
            .b = this->b + other.b,
        };
    }
    
    inline void setIndexColour(size_t index, int value) {
        switch (index) {
            case 0:
                y = value;
                break;
            case 1:
                cb = value;
                break;
            case 2:
                cr = value;
                break;
            default:
                throw std::logic_error("index out of bounds for colour");
                break;
        }
    }
};

struct PixelBuffer {
public:
    typedef int32_t size_type;
    typedef std::vector<size_type> PixelVector;
    static const size_t BlockSize = 8;

private:
    size_t _width = 0;
    size_t _height = 0;
    size_t _block_span = 0;
    PixelVector _buffer;

public:
    PixelBuffer() {}
    PixelBuffer(size_t width, size_t height) {
        resize(width, height);
    }

    void resize(size_t width, size_t height) {
        _width = width;
        _height = height;
        _block_span = _width / BlockSize;
        _buffer.resize(width * height);
    }

    size_t width() const {
        return _width;
    }

    size_t height() const {
        return _height;
    }

    size_t span_mb() const {
        return _block_span;
    }

    constexpr size_type& operator[](size_t index) {
        return _buffer[index];
    }

    constexpr size_type& at(size_t index) {
        return _buffer.at(index);
    }

    constexpr size_type at(size_t index) const {
        return _buffer.at(index);
    }

    PixelVector::iterator begin() {
        return _buffer.begin();
    }

    PixelVector::iterator end() {
        return _buffer.end();
    }
};

struct Frame {
    size_t encoded_width = 0;
    size_t encoded_height = 0;
    PixelBuffer luma;
    PixelBuffer cb;
    PixelBuffer cr;
    std::string description;

    void resize(size_t width, size_t height) {
        encoded_width = width;
        encoded_height = height;
        luma.resize(encoded_width, encoded_height);
        cb.resize(encoded_width / 2, encoded_height / 2);
        cr.resize(encoded_width / 2, encoded_height / 2);
    }

    static Frame make_frame(size_t width, size_t height) {
        return Frame{
            .encoded_width = width,
            .encoded_height = height,
            .luma = PixelBuffer(width, height),
            .cb = PixelBuffer(width / 2, height / 2),
            .cr = PixelBuffer(width / 2, height / 2)
        };
    }

    std::vector<std::array<float, 4>> as_float_rgba_vector() const;
};

Colour ycbcrToRGB(const Colour& ycbcr);
void ycbcrToRGBOverMCU(Colour* data, size_t width, size_t x, size_t y);

void writeOutPPM(std::ostream& out, Frame& data);
void writeOutPPM(std::string filepath, Frame& data);

}
