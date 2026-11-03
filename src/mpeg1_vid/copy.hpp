
//------------------------------------------------------------------------------
// copy.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"
#include "mpeg1.hpp"

#include <array>
#include <vector>

namespace mpeg1 {

void copy_block_to_buff(int mb_addr, mpeg1::Block& block, image::PixelBuffer& frame);
void add_block_to_buff(int mb_addr, mpeg1::Block& block, image::PixelBuffer& frame);
void copy_mv_block(int addr, int32_t recon_right_for, int32_t recon_down_for, const image::Frame& source, image::Frame& destination);

}