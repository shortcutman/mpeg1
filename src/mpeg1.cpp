
//------------------------------------------------------------------------------
// mpeg1.cpp
//------------------------------------------------------------------------------

#include "mpeg1.hpp"

#include "bitspan.hpp"
#include "vlc.hpp"

mpeg1::SequenceHeader mpeg1::read_sequence_header(std::span<std::byte>& data) {
    SequenceHeader header;

    util::bitspan bits(data);

    if (bits.read_bits_be(32) != mpeg1::start_code::sequence) {
        throw std::runtime_error("Expected sequence header start code!");
    }

    header.horizontal_size = bits.read_bits_be(12);
    header.vertical_size = bits.read_bits_be(12);

    auto pel_aspect_ratio_value = bits.read_bits_be(4);
    header.pel_aspect_ratio = mpeg1::PEL_ASPECT_RATIO_TABLE[pel_aspect_ratio_value];

    auto picture_rate_value = bits.read_bits_be(4);
    header.picture_rate = mpeg1::PICTURE_RATE_TABLE[picture_rate_value];

    header.bit_rate = bits.read_bits_be(18) * 400;

    auto marker_bit = bits.read_bits_be(1);
    if (!marker_bit) {
        throw std::runtime_error("Marker bit is not set.");
    }

    bits.read_bits_be(10); //vbv_buffer_size
    bits.read_bits_be(1); //constrained_parameters_flag

    auto load_intra_quantizer_matrix = bits.read_bits_be(1);
    if (load_intra_quantizer_matrix) {
        for (uint8_t i = 0; i < 64; i++) {
            header.intra_quantizer_matrix[ZIGZAG_INDEX[i]] = bits.read_bits_be(8);
        }
    }

    auto load_non_intra_quantizer_matrix = bits.read_bits_be(1);
    if (load_non_intra_quantizer_matrix) {
        for (uint8_t i = 0; i < 64; i++) {
            header.non_intra_quantizer_matrix[ZIGZAG_INDEX[i]] = bits.read_bits_be(8);
        }
    }

    data = data.subspan(bits.bytes_read());

    return header;
}

mpeg1::GroupOfPicturesHeader mpeg1::read_gop_header(std::span<std::byte>& data) {
    GroupOfPicturesHeader header;

    util::bitspan bits(data);

    if (bits.read_bits_be(32) != mpeg1::start_code::group_of_pictures) {
        throw std::runtime_error("Expected group of pictures header start code!");
    }

    header.time_code = bits.read_bits_be(25);
    header.closed_gop = bits.read_bits_be(1);
    header.broken_link = bits.read_bits_be(1);

    data = data.subspan(bits.bytes_read());

    return header;
}

mpeg1::PictureHeader mpeg1::read_picture_header(std::span<std::byte>& data) {
    PictureHeader header;
    util::bitspan bits(data);

    if (bits.read_bits_be(32) != mpeg1::start_code::picture) {
        throw std::runtime_error("Expected picture header start code!");
    }

    header.temporal_reference = bits.read_bits_be(10);
    
    auto coding_type = bits.read_bits_be(3);
    switch (coding_type) {
        case 1:
            header.coding_type = CodingType::IntraCoded;
            break;
        case 2:
            header.coding_type = CodingType::PredictiveCoded;
            break;
        case 3:
            header.coding_type = CodingType::BidirectionalPredCoded;
            break;
        
        default:
            throw std::runtime_error("Unsupported picture coding type.");
            break;
    }

    header.full_pel_forward_vector = bits.read_bits_be(1);
    header.forward_f_code = bits.read_bits_be(3);
    header.full_pel_backward_vector = bits.read_bits_be(1);
    header.backward_f_code = bits.read_bits_be(3);

    return header;
}

mpeg1::SliceHeader mpeg1::read_slice_header(util::bitspan& data) {
    SliceHeader header;

    auto start_code = data.read_bits_be(32);
    if (start_code < mpeg1::start_code::slice_minimum || start_code > mpeg1::start_code::slice_maximum) {
        throw std::runtime_error("Expected slice start code in valid range!");
    }

    header.vertical_position = start_code & 0xff;
    header.quantizer_scale = data.read_bits_be(5);

    while (data.read_bits_be(1) == 1) {
        data.read_bits_be(8);
    }

    return header;
}

mpeg1::Macroblock mpeg1::read_macroblock(util::bitspan& data, CodingType coding_type) {
    Macroblock block;

    while (data.peek_bits_be(11) == 0x00f) {
        data.read_bits_be(11);
    }

    while (data.peek_bits_be(11) == 0x008) {
        data.read_bits_be(11);
        block.address_increment += 33;
    }

    block.address_increment = mpeg1::MACROBLOCK_ADDRESSING.next_symbol(data);

    switch (coding_type) {
        case CodingType::IntraCoded: {
            auto type_index = mpeg1::MACROBLOCK_TYPE_INTRA_VLC.next_symbol(data);
            auto& type_def = mpeg1::MACROBLOCK_TYPE_INTRA_DEFS[type_index];
            block.quant = type_def.quant;
            block.motion_forward = type_def.motion_forward;
            block.motion_backward = type_def.motion_backward;
            block.motion_pattern = type_def.pattern;
            block.intra = type_def.intra;
        }
        break;

        default: {
            throw std::runtime_error("Unhandled CodingType for macroblock.");
        }
        break;
    }

    return block;
}
