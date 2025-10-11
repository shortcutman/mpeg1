
//------------------------------------------------------------------------------
// colour.hpp
//------------------------------------------------------------------------------

#pragma once

#include <span>
#include <tuple>

namespace image {

struct Colour {
    union {
        int r;
        int y;
    };
    
    union {
        int g;
        int cb;
    };
    
    union {
        int b;
        int cr;
    };
    
    bool operator==(const Colour& b) const {
        return this->r == b.r &&
               this->g == b.g &&
               this->b == b.b;
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
