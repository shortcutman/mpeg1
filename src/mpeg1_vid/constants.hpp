
//------------------------------------------------------------------------------
// constants.hpp
//------------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstdint>

namespace mpeg1 {

    namespace start_code {
        extern const uint32_t sequence;
        extern const uint32_t group_of_pictures;
        extern const uint32_t picture;
        extern const uint32_t slice_minimum;
        extern const uint32_t slice_maximum;
    }

    typedef std::array<uint8_t, 64> QuantizerMatrix;
    extern const QuantizerMatrix DEFAULT_INTRA_QUANTIZER_MATRIX;
    extern const QuantizerMatrix DEFAULT_NON_INTRA_QUANTIZER_MATRIX;

    extern const std::array<float, 16> PEL_ASPECT_RATIO_TABLE;
    extern const std::array<float, 16> PICTURE_RATE_TABLE;

    extern const std::array<uint8_t, 64> ZIGZAG_INDEX;
}
