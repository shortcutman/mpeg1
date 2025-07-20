
//------------------------------------------------------------------------------
// mpeg1.consts.cpp
//------------------------------------------------------------------------------

#include "mpeg1.consts.hpp"

#include <limits>

namespace mpeg1 {

const QuantizerMatrix DEFAULT_INTRA_QUANTIZER_MATRIX = {
    8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

const QuantizerMatrix DEFAULT_NON_INTRA_QUANTIZER_MATRIX = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
};

const std::array<float, 16> PEL_ASPECT_RATIO_TABLE = {
    std::numeric_limits<float>::signaling_NaN(),
    1.0f,
    0.6735f,
    0.7031f,
    0.7651f,
    0.8055f,
    0.8437f,
    0.8935f,
    0.9157f,
    0.9815f,
    1.0255f,
    1.0695f,
    1.0950f,
    1.1575f,
    1.2015f,
    std::numeric_limits<float>::signaling_NaN()
};

const std::array<float, 16> PICTURE_RATE_TABLE = {
    std::numeric_limits<float>::signaling_NaN(),
    23.976f,
    24.f,
    25.f,
    29.97f,
    30.f,
    50.f,
    59.94f,
    60.f,
    std::numeric_limits<float>::signaling_NaN(),
    std::numeric_limits<float>::signaling_NaN(),
    std::numeric_limits<float>::signaling_NaN(),
    std::numeric_limits<float>::signaling_NaN(),
    std::numeric_limits<float>::signaling_NaN(),
    std::numeric_limits<float>::signaling_NaN(),
    std::numeric_limits<float>::signaling_NaN()
};

}
