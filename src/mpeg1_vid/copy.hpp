
//------------------------------------------------------------------------------
// copy.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"
#include "mpeg1.hpp"

#include <array>
#include <vector>

namespace mpeg1 {

void copy_mb_to_image(int addr, const std::array<image::Colour, 256>& block, image::Frame& frame);

void copy_block_to_y(int mb_addr, int block_num, std::array<int, 8*8>& block, image::Frame& frame);
void copy_block_to_buff(int mb_addr, mpeg1::Block& block, image::PixelBuffer& frame);
void add_block_to_y(int mb_addr, int block_num, std::array<int, 8*8>& block, image::Frame& frame);

std::array<image::Colour, 256> copy_block_mv_from_image(int addr, std::tuple<int, int> motion_vector, const image::Frame& source);

void copy_mv_block(int addr, std::tuple<int, int> motion_vector, const image::Frame& source, image::Frame& destination);

}