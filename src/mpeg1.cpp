
//------------------------------------------------------------------------------
// mpeg1.cpp
//------------------------------------------------------------------------------

#include "mpeg1.hpp"

#include "bitspan.hpp"

#include <limits>
#include <print>

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
            header.intra_quantizer_matrix[dezigzag(i)] = bits.read_bits_be(8);
        }
    }

    auto load_non_intra_quantizer_matrix = bits.read_bits_be(1);
    if (load_non_intra_quantizer_matrix) {
        for (uint8_t i = 0; i < 64; i++) {
            header.non_intra_quantizer_matrix[dezigzag(i)] = bits.read_bits_be(8);
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

constexpr uint8_t mpeg1::dezigzag(uint8_t index) {
    const uint8_t zigzagTable[] = {
        0,   1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63,
    };
        
    return zigzagTable[index];
}
