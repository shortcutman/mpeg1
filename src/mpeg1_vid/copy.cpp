
//------------------------------------------------------------------------------
// copy.cpp
//------------------------------------------------------------------------------

#include "copy.hpp"

#include <algorithm>

void mpeg1::copy_mb_to_image(int addr, const std::array<image::Colour, 256>& block, image::Frame& frame) {
    const size_t span = frame.encoded_width;
    const size_t span_mb = span / 16;

    // addr -= 1; //addr starts from 1
    auto addrHorizontal = addr % span_mb;
    auto addrVertical = addr / span_mb;
    auto imageStartIt = frame.image.begin() + addrHorizontal * 16 + addrVertical * 16 * span;
    for (size_t i = 0; i < 16; i++) {
        auto blockIt = block.begin() + i * 16;
        auto imageIt = imageStartIt + i * span;
        std::copy(blockIt, blockIt + 16, imageIt);
    }
}

std::array<image::Colour, 256> mpeg1::copy_block_mv_from_image(int addr, std::tuple<int, int> motion_vector, const image::Frame& source) {
    size_t span = source.encoded_width;
    std::array<image::Colour, 256> block;
    std::fill_n(block.begin(), 256, image::Colour{});

    auto right_for = std::get<0>(motion_vector) >> 1;
    auto down_for = std::get<1>(motion_vector) >> 1;
    auto right_half_for = std::get<0>(motion_vector) - 2 * right_for;
    auto down_half_for = std::get<1>(motion_vector) - 2 * down_for;

    auto addrHor = (addr % (span / 16)) * 16;
    auto addrVer = (addr / (span / 16)) * 16;

    //implement copy of pels as per bottom of page 35
    if (!right_half_for && !down_half_for) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16] = source.image.at(addrHor + right_for + x + (addrVer + down_for + y) * span);
            }
        }
    } else if (!right_half_for && down_half_for) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16] =
                    (source.image.at(addrHor + right_for + x + (addrVer + down_for + y) * span) +
                    source.image.at(addrHor + right_for + x + (addrVer + down_for + y + 1) * span)) / 2;
            }
        }
    } else if (right_half_for && !down_half_for) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16] =
                    (source.image.at(addrHor + right_for + x + (addrVer + down_for + y) * span) +
                    source.image.at(addrHor + right_for + x + 1 + (addrVer + down_for + y) * span)) / 2;
            }
        }
    } else {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16] =
                    (source.image.at(addrHor + right_for + x + (addrVer + down_for + y) * span) +
                    source.image.at(addrHor + right_for + x + 1 + (addrVer + down_for + y) * span) +
                    source.image.at(addrHor + right_for + x + (addrVer + down_for + y + 1) * span) +
                    source.image.at(addrHor + right_for + x + 1 + (addrVer + down_for + y + 1) * span)) / 4;
            }
        }
    }

    return block;
}
