
//------------------------------------------------------------------------------
// copy.cpp
//------------------------------------------------------------------------------

#include "copy.hpp"

#include "mpeg1.hpp"

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

    auto addrHor = (addr % (span / 16)) * 16;
    auto addrVer = (addr / (span / 16)) * 16;

    auto [recon_right_for, recon_down_for] = motion_vector;

    auto right_for_y = recon_right_for >> 1;
    auto down_for_y = recon_down_for >> 1;
    auto right_half_for_y = recon_right_for - 2 * right_for_y;
    auto down_half_for_y = recon_down_for - 2 * down_for_y;

    //implement copy of pels as per bottom of page 35
    if (!right_half_for_y && !down_half_for_y) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16].y = source.image.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span).y;
            }
        }
    } else if (!right_half_for_y && down_half_for_y) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16].y =
                    (source.image.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span).y +
                    source.image.at(addrHor + right_for_y + x + (addrVer + down_for_y + y + 1) * span).y +
                    1) / 2;
            }
        }
    } else if (right_half_for_y && !down_half_for_y) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16].y =
                    (source.image.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span).y +
                    source.image.at(addrHor + right_for_y + x + 1 + (addrVer + down_for_y + y) * span).y +
                    1) / 2;
            }
        }
    } else {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                block[x + y*16].y =
                    (source.image.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span).y +
                    source.image.at(addrHor + right_for_y + x + 1 + (addrVer + down_for_y + y) * span).y +
                    source.image.at(addrHor + right_for_y + x + (addrVer + down_for_y + y + 1) * span).y +
                    source.image.at(addrHor + right_for_y + x + 1 + (addrVer + down_for_y + y + 1) * span).y +
                    2) / 4;
            }
        }
    }

    auto right_for_c = (recon_right_for / 2) >> 1;
    auto down_for_c = (recon_down_for / 2) >> 1;
    auto right_half_for_c = (recon_right_for / 2) - (2 * right_for_c);
    auto down_half_for_c = (recon_down_for / 2) - (2 * down_for_c);

    mpeg1::Block block_cb, block_cr;
    block_cb.fill(0);
    block_cr.fill(0);
    if (!right_half_for_c && !down_half_for_c) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                block_cb[x + y*8] = source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y) * 2) * span).cb;
                block_cr[x + y*8] = source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y) * 2) * span).cr;
            }
        }
    } else if (!right_half_for_c && down_half_for_c) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                auto c =
                    (source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y) * 2) * span) +
                    source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y + 1) * 2) * span) +
                    image::Colour{.cb = 1, .cr = 1}) / 2;
                block_cb[x + y*8] = c.cb;
                block_cr[x + y*8] = c.cr;
                (void)c;
            }
        }
    } else if (right_half_for_c && !down_half_for_c) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                auto c =
                    (source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y) * 2) * span) +
                    source.image.at(addrHor + (right_for_c + x + 1) * 2 + (addrVer + (down_for_c + y) * 2) * span) +
                    image::Colour{.cb = 1, .cr = 1}) / 2;
                block_cb[x + y*8] = c.cb;
                block_cr[x + y*8] = c.cr;
                (void)c;
            }
        }
    } else {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                auto c =
                    (source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y) * 2) * span) +
                    source.image.at(addrHor + (right_for_c + x + 1) * 2 + (addrVer + (down_for_c + y) * 2) * span) +
                    source.image.at(addrHor + (right_for_c + x) * 2 + (addrVer + (down_for_c + y + 1) * 2) * span) +
                    source.image.at(addrHor + (right_for_c + x + 1) * 2 + (addrVer + (down_for_c + y + 1) * 2) * span) +
                    image::Colour{.cb = 2, .cr = 2}) / 4;
                block_cb[x + y*8] = c.cb;
                block_cr[x + y*8] = c.cr;
                (void)c;
            }
        }
    }

    mpeg1::assign_to_cb(block_cb, block);
    mpeg1::assign_to_cr(block_cr, block);

    return block;
}
