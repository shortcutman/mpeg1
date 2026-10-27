
//------------------------------------------------------------------------------
// colour.cpp
//------------------------------------------------------------------------------

#include "colour.hpp"

#include <fstream>

using namespace image;

namespace {

float adjustAndClamp(float val) {
    if (val > 255.f) {
        return 1.f;
    } else if (val < 0.f) {
        return 0.f;
    } else {
        return val / 255.f;
    }
}

}

std::vector<std::array<float, 4>> image::Frame::as_float_rgba_vector() const {
    std::vector<std::array<float, 4>> img_float;

    for (size_t y = 0; y < encoded_height; y++) {
        for (size_t x = 0; x < encoded_width; x++) {
            size_t yIdx = y * luma.width() + x;
            size_t cIdx = y / 2 * cb.width() + x / 2;

            auto r = adjustAndClamp(1.164f * (luma.at(yIdx) - 16) + 1.596f * (cr.at(cIdx) - 128));
            auto g = adjustAndClamp(1.164f * (luma.at(yIdx) - 16) - 0.813f * (cr.at(cIdx) - 128) - 0.391f * (cb.at(cIdx) - 128));
            auto b = adjustAndClamp(1.164f * (luma.at(yIdx) - 16) + 2.018f * (cb.at(cIdx) - 128));
            img_float.push_back({r, g, b, 1.f});
        }
    }

    return img_float;
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

void image::writeOutPPM(std::ostream& out, Frame& data) {    

    auto rgba = data.as_float_rgba_vector();

    out << "P3" << std::endl;
    out << data.encoded_width << " " << data.encoded_height << std::endl;
    out << "255" << std::endl;
     
    for (size_t y = 0; y < data.encoded_height; y++) {
        for (size_t x = 0; x < data.encoded_width; x++) {
            auto pixel = rgba[x + y * data.encoded_width];
            out << std::format("{:.0f}", pixel[0] * 255) << " "
                << std::format("{:.0f}", pixel[1] * 255) << " "
                << std::format("{:.0f}", pixel[2] * 255) << " ";
        }

        out << std::endl;
    }
}

void image::writeOutPPM(std::string filepath, Frame& data) {
    std::ofstream file;
    file.open(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing.");
    }

    writeOutPPM(file, data);
    
    file.close();
}
