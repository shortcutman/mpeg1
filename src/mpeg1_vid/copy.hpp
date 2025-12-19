
//------------------------------------------------------------------------------
// copy.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"

#include <array>
#include <vector>

namespace mpeg1 {

void copy_mb_to_image(int addr, const std::array<image::Colour, 256>& block, image::Frame& frame);

std::array<image::Colour, 256> copy_block_mv_from_image(int addr, std::tuple<int, int> motion_vector, const image::Frame& source);

}