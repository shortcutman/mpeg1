
//------------------------------------------------------------------------------
// mpeg1.consts.hpp
//------------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstdint>

namespace mpeg1 {
    typedef std::array<uint8_t, 64> QuantizerMatrix;
    extern const QuantizerMatrix DEFAULT_INTRA_QUANTIZER_MATRIX;
    extern const QuantizerMatrix DEFAULT_NON_INTRA_QUANTIZER_MATRIX;

    extern const std::array<float, 16> PEL_ASPECT_RATIO_TABLE;
    extern const std::array<float, 16> PICTURE_RATE_TABLE;
}