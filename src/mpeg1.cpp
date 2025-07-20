
//------------------------------------------------------------------------------
// mpeg1.cpp
//------------------------------------------------------------------------------

#include "mpeg1.hpp"

#include "bitspan.hpp"


mpeg1::SequenceHeader mpeg1::read_sequence_header(std::span<std::byte>& data) {
    SequenceHeader header;

    util::bitspan bits(data);

    if (bits.read_bits_be(32) != 0x000001B3) {
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

    auto bytes_read = bits.bits_read() / 8;
    bytes_read += bits.bits_read() % 8 ? 1 : 0;
    data = data.subspan(bytes_read);

    return header;
}

mpeg1::GroupOfPicturesHeader mpeg1::read_gop_header(std::span<std::byte>& data) {
    GroupOfPicturesHeader header;

    util::bitspan bits(data);

    if (bits.read_bits_be(32) != 0x000001b8) {
        throw std::runtime_error("Expected sequence header start code!");
    }

    header.time_code = bits.read_bits_be(25);
    header.closed_gop = bits.read_bits_be(1);
    header.broken_link = bits.read_bits_be(1);

    auto bytes_read = bits.bits_read() / 8;
    bytes_read += bits.bits_read() & 8 ? 1 : 0;
    data = data.subspan(bytes_read);

    return header;
}
