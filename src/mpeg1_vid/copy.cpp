
//------------------------------------------------------------------------------
// copy.cpp
//------------------------------------------------------------------------------

#include "copy.hpp"

#include <algorithm>

void mpeg1::copy_block_to_buff(int mb_addr, mpeg1::Block& block, image::PixelBuffer& destination) {
    size_t addrHorizontal = mb_addr % destination.span_mb();
    size_t addrVertical = mb_addr / destination.span_mb();

    auto outIt = destination.begin() + addrHorizontal * 8 + addrVertical * 8 * destination.width();

    for (auto i = 0; i < 8; i++) {
        std::copy(block.begin() + i * 8, block.begin() + i * 8 + 8, outIt);
        outIt += destination.width();
    }
}

void mpeg1::add_block_to_buff(int mb_addr, mpeg1::Block& block, image::PixelBuffer& destination) {
    size_t addrHorizontal = mb_addr % destination.span_mb();
    size_t addrVertical = mb_addr / destination.span_mb();

    auto outIt = destination.begin() + addrHorizontal * 8 + addrVertical * 8 * destination.width();

    for (auto i = 0; i < 8; i++) {
        *(outIt++) += block[i*8 + 0];
        *(outIt++) += block[i*8 + 1];
        *(outIt++) += block[i*8 + 2];
        *(outIt++) += block[i*8 + 3];
        *(outIt++) += block[i*8 + 4];
        *(outIt++) += block[i*8 + 5];
        *(outIt++) += block[i*8 + 6];
        *(outIt++) += block[i*8 + 7];
        outIt += destination.width() - 8;
    }
}

void mpeg1::copy_mv_block(int addr, std::tuple<int, int> motion_vector, const image::Frame& source, image::Frame& destination) {
    size_t span = source.encoded_width;

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
                destination.luma[addrHor + x + (addrVer + y) * span] =
                    source.luma.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span);
            }
        }
    } else if (!right_half_for_y && down_half_for_y) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                destination.luma[addrHor + x + (addrVer + y) * span] =
                    (source.luma.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span) +
                    source.luma.at(addrHor + right_for_y + x + (addrVer + down_for_y + y + 1) * span) +
                    1) / 2;
            }
        }
    } else if (right_half_for_y && !down_half_for_y) {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                destination.luma[addrHor + x + (addrVer + y) * span] =
                    (source.luma.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span) +
                    source.luma.at(addrHor + right_for_y + x + 1 + (addrVer + down_for_y + y) * span) +
                    1) / 2;
            }
        }
    } else {
        for (size_t y = 0; y < 16; y++) {
            for (size_t x = 0; x < 16; x++) {
                destination.luma[addrHor + x + (addrVer + y) * span] =
                    (source.luma.at(addrHor + right_for_y + x + (addrVer + down_for_y + y) * span) +
                    source.luma.at(addrHor + right_for_y + x + 1 + (addrVer + down_for_y + y) * span) +
                    source.luma.at(addrHor + right_for_y + x + (addrVer + down_for_y + y + 1) * span) +
                    source.luma.at(addrHor + right_for_y + x + 1 + (addrVer + down_for_y + y + 1) * span) +
                    2) / 4;
            }
        }
    }

    //chroma blue and red
    auto right_for_c = (recon_right_for / 2) >> 1;
    auto down_for_c = (recon_down_for / 2) >> 1;
    auto right_half_for_c = (recon_right_for / 2) - (2 * right_for_c);
    auto down_half_for_c = (recon_down_for / 2) - (2 * down_for_c);

    addrHor /= 2;
    addrVer /= 2;
    span /= 2;

    if (!right_half_for_c && !down_half_for_c) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                destination.cb.at(addrHor + x + (addrVer + y) * span) =
                    source.cb.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span);
                destination.cr.at(addrHor + x + (addrVer + y) * span) =
                    source.cr.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span);
            }
        }
    } else if (!right_half_for_c && down_half_for_c) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                destination.cb.at(addrHor + x + (addrVer + y) * span) = 
                    (source.cb.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y) ) * span) +
                    source.cb.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y + 1)) * span) +
                    1) / 2;
                destination.cr.at(addrHor + x + (addrVer + y) * span) = 
                    (source.cr.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span) +
                    source.cr.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y + 1)) * span) +
                    1) / 2;
            }
        }
    } else if (right_half_for_c && !down_half_for_c) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                destination.cb.at(addrHor + x + (addrVer + y) * span) = 
                    (source.cb.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span) +
                    source.cb.at(addrHor + (right_for_c + x + 1) + (addrVer + (down_for_c + y)) * span) +
                    1) / 2;
                destination.cr.at(addrHor + x + (addrVer + y) * span) = 
                    (source.cr.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span) +
                    source.cr.at(addrHor + (right_for_c + x + 1) + (addrVer + (down_for_c + y)) * span) +
                    1) / 2;
            }
        }
    } else {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                destination.cb.at(addrHor + x + (addrVer + y) * span) = 
                    (source.cb.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span) +
                    source.cb.at(addrHor + (right_for_c + x + 1) + (addrVer + (down_for_c + y)) * span) +
                    source.cb.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y + 1)) * span) +
                    source.cb.at(addrHor + (right_for_c + x + 1) + (addrVer + (down_for_c + y + 1)) * span) +
                    2) / 4;
                destination.cr.at(addrHor + x + (addrVer + y) * span) = 
                    (source.cr.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y)) * span) +
                    source.cr.at(addrHor + (right_for_c + x + 1) + (addrVer + (down_for_c + y)) * span) +
                    source.cr.at(addrHor + (right_for_c + x) + (addrVer + (down_for_c + y + 1)) * span) +
                    source.cr.at(addrHor + (right_for_c + x + 1) + (addrVer + (down_for_c + y + 1)) * span) +
                    2) / 4;
            }
        }
    }
}