
//------------------------------------------------------------------------------
// colour.hpp
//------------------------------------------------------------------------------

#pragma once

#include <span>
#include <tuple>

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

Colour ycbcrToRGB(const Colour& ycbcr);
void ycbcrToRGBOverMCU(Colour* data, size_t width, size_t x, size_t y);

void writeOutPPM(std::string filepath, size_t width, size_t height, std::span<Colour> data);

}
