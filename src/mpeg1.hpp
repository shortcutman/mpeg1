
//------------------------------------------------------------------------------
// mpeg1.hpp
//------------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace mpeg1 {
    typedef std::array<uint8_t, 64> QuantizerMatrix;

    extern const QuantizerMatrix DEFAULT_INTRA_QUANTIZER_MATRIX;
    extern const QuantizerMatrix DEFAULT_NON_INTRA_QUANTIZER_MATRIX;

    struct SequenceHeader {
        uint16_t horizontal_size;
        uint16_t vertical_size;
        float pel_aspect_ratio;
        float picture_rate;
        uint64_t bit_rate;
        QuantizerMatrix intra_quantizer_matrix = DEFAULT_INTRA_QUANTIZER_MATRIX;
        QuantizerMatrix non_intra_quantizer_matrix = DEFAULT_NON_INTRA_QUANTIZER_MATRIX;
    };

    struct GroupOfPicturesHeader {
        uint32_t time_code;
        bool closed_gop;
        bool broken_link;
    };

    SequenceHeader read_sequence_header(std::span<std::byte>& data);
    GroupOfPicturesHeader read_gop_header(std::span<std::byte>& data);

    constexpr uint8_t dezigzag(uint8_t index);
}
