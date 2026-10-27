
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
void copy_mv_block(int addr, std::tuple<int, int> motion_vector, const image::Frame& source, image::Frame& destination);

}