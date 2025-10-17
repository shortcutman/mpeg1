
//------------------------------------------------------------------------------
// colour.cpp
//------------------------------------------------------------------------------

#include "colour.hpp"

#include <fstream>

using namespace image;

namespace {

inline int adjustAndClamp(float val) {
    if (val > 255) {
        return 255;
    } else if (val < 0) {
        return 0;
    } else {
        return static_cast<int>(val);
    }
}

}

// MPEG1 uses ITU-R Recommendation BT.601 https://en.wikipedia.org/wiki/Rec._601
// Conversion to RGB from https://fourcc.org/fccyvrgb.php
Colour image::ycbcrToRGB(const Colour& ycbcr) {
    auto r = adjustAndClamp(1.164f * (ycbcr.y - 16) + 1.596f * (ycbcr.cr - 128));
    auto g = adjustAndClamp(1.164f * (ycbcr.y - 16) - 0.813f * (ycbcr.cr - 128) - 0.391f * (ycbcr.cb - 128));
    auto b = adjustAndClamp(1.164f * (ycbcr.y - 16) + 2.018f * (ycbcr.cb - 128));
    return Colour{.r = (int)r, .g = (int)g, .b = (int)b};
}

void image::ycbcrToRGBOverMCU(Colour *data, size_t width, size_t xStart, size_t yStart) {
    for (size_t y = yStart; y < (yStart + 16); y++) {
        for (size_t x = xStart; x < (xStart + 16); x++) {
            data[y * width + x] = ycbcrToRGB(data[y * width + x]);
        }
    }
}

void image::writeOutPPM(std::string filepath, size_t width, size_t height, std::span<Colour> data) {
    std::ofstream file;
    file.open(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing.");
    }
    
    if (width * height > data.size()) {
        throw std::runtime_error("Width and height greater than provided data.");
    }
    
    file << "P3" << std::endl;
    file << width << " " << height << std::endl;
    file << "255" << std::endl;
     
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            auto pixel = data[x + y * width];
            file << std::to_string(pixel.r) << " "
                 << std::to_string(pixel.g) << " "
                 << std::to_string(pixel.b) << " ";
        }
         
        file << std::endl;
    }
    
    file.close();
}
