
//------------------------------------------------------------------------------
// mpeg1.hpp
//------------------------------------------------------------------------------

#pragma once

#include "mpeg1.consts.hpp"

#include "bitspan.hpp"

#include <cstdint>
#include <span>

namespace mpeg1 {
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

    enum class CodingType {
        IntraCoded,
        PredictiveCoded,
        BidirectionalPredCoded
    };

    struct PictureHeader {
        uint16_t temporal_reference;
        CodingType coding_type;
        
        bool full_pel_forward_vector;
        uint8_t forward_f_code;

        bool full_pel_backward_vector;
        uint8_t backward_f_code;
    };

    struct SliceHeader {
        uint8_t vertical_position;
        uint8_t quantizer_scale;
    };

    struct Macroblock {
        uint8_t address_increment = 0;

        bool quant;
        bool motion_forward;
        bool motion_backward;
        bool motion_pattern;
        bool intra;
    };

    SequenceHeader read_sequence_header(std::span<std::byte>& data);
    GroupOfPicturesHeader read_gop_header(std::span<std::byte>& data);
    PictureHeader read_picture_header(std::span<std::byte>& data);

    SliceHeader read_slice_header(util::bitspan& data);
    Macroblock read_macroblock(util::bitspan& data);
}
