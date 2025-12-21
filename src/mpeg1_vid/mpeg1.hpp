
//------------------------------------------------------------------------------
// mpeg1.hpp
//------------------------------------------------------------------------------

#pragma once

#include "mpeg1_vid/constants.hpp"

#include "bitspan.hpp"
#include "colour.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace mpeg1 {
    struct SequenceHeader {
        uint16_t horizontal_size = 0;
        uint16_t vertical_size = 0;
        float pel_aspect_ratio = 0;
        float picture_rate = 0;
        uint64_t bit_rate = 0;
        QuantizerMatrix intra_quantizer_matrix = DEFAULT_INTRA_QUANTIZER_MATRIX;
        QuantizerMatrix non_intra_quantizer_matrix = DEFAULT_NON_INTRA_QUANTIZER_MATRIX;

        size_t mb_width() const { return (horizontal_size + 15) / 16; }
        size_t mb_height() const { return (vertical_size + 15) / 16; }

        size_t encoded_width() const { return mb_width() * 16; }
        size_t encoded_height() const { return mb_height() * 16; }
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

    std::string ct_to_string(CodingType type);

    struct PictureHeader {
        uint16_t temporal_reference;
        CodingType coding_type;
        uint16_t vbv_delay;
        
        bool full_pel_forward_vector;
        uint8_t forward_f_code;

        bool full_pel_backward_vector;
        uint8_t backward_f_code;
    };

    struct SliceHeader {
        uint32_t vertical_position;
        uint8_t quantizer_scale;
    };

    struct MacroblockType {
        bool quant;
        bool motion_forward;
        bool motion_backward;
        bool pattern;
        bool intra;
    };

    typedef std::array<image::Colour, 256> MacroblockData;
    typedef std::array<int, 64> Block;

    struct Macroblock {
        uint8_t address_increment = 0;
        MacroblockType type;

        uint8_t quantizer_scale = 0;
        
        int32_t motion_horizontal_forward_code = 0;
        uint32_t motion_horizontal_forward_r = 0;
        int32_t motion_vertical_forward_code = 0;
        uint32_t motion_vertical_forward_r = 0;

        int8_t motion_horizontal_backward_code = 0;
        uint8_t motion_horizontal_backward_r = 0;
        int8_t motion_vertical_backward_code = 0;
        uint8_t motion_vertical_backward_r = 0;

        uint32_t coded_block_pattern = 0;
    };

    struct BlockContext {
        SequenceHeader sequence;
        SliceHeader slice;

        int previous_macroblock_address;
        int macroblock_address;

        int dct_dc_y_past = 1024;
        int dct_dc_cb_past = 1024;
        int dct_dc_cr_past = 1024;

        int past_intra_address = -2;
        int mv_right_for_prev = 0;
        int mv_down_for_prev = 0;
    };

    SequenceHeader read_sequence_header(std::span<std::byte>& data);
    GroupOfPicturesHeader read_gop_header(std::span<std::byte>& data);
    PictureHeader read_picture_header(std::span<std::byte>& data);

    SliceHeader read_slice_header(util::bitspan& data);
    Macroblock read_macroblock(util::bitspan& data, const PictureHeader& picture);

    std::array<image::Colour, 256> read_intra_blocks(util::bitspan& data, BlockContext& context);

    std::array<int, 64> read_block(util::bitspan& data, BlockContext& context, size_t block_index);

    std::tuple<int, int> calc_motion_vectors(const PictureHeader& picture, const Macroblock& macroblock, std::tuple<int, int> prev);
    size_t calc_dct_zz_zero(size_t dc_size, size_t dc_differential);

    bool check_cbp(uint32_t coded_block_pattern, size_t index);

    void assign_to_y(const Block& block, MacroblockData& macroblock, size_t index);
    void assign_to_cb(const Block& block, MacroblockData& macroblock);
    void assign_to_cr(const Block& block, MacroblockData& macroblock);
}
