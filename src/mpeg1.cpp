
//------------------------------------------------------------------------------
// mpeg1.cpp
//------------------------------------------------------------------------------

#include "mpeg1.hpp"

#include "bitspan.hpp"
#include "idct.hpp"
#include "mpeg1.vlc.hpp"
#include "util.hpp"

#include <print>

std::string mpeg1::ct_to_string(mpeg1::CodingType type) {
    switch (type) {
        case CodingType::IntraCoded:
            return "IntraCoded";
        case CodingType::PredictiveCoded:
            return "PredictiveCoded";
        case CodingType::BidirectionalPredCoded:
            return "BidirectionalPredCoded";
        default:
            return "Unknown";
    }
}

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

    header.vbv_delay = bits.read_bits_be(16);

    if (header.coding_type == CodingType::PredictiveCoded ||
        header.coding_type == CodingType::BidirectionalPredCoded) {
        header.full_pel_forward_vector = bits.read_bits_be(1);
        header.forward_f_code = bits.read_bits_be(3);
    }

    if (header.coding_type == CodingType::BidirectionalPredCoded) {
        header.full_pel_backward_vector = bits.read_bits_be(1);
        header.backward_f_code = bits.read_bits_be(3);
    }

    data = data.subspan(bits.bytes_read());

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
            auto type = mpeg1::MACROBLOCK_TYPE_INTRA_VLC.next_symbol(data);
            block.quant = type.quant;
            block.motion_forward = type.motion_forward;
            block.motion_backward = type.motion_backward;
            block.pattern = type.pattern;
            block.intra = type.intra;
        }
        break;

        default: {
            throw std::runtime_error("Unhandled CodingType for macroblock.");
        }
        break;
    }

    if (block.quant ||
        block.motion_forward ||
        block.motion_backward ||
        block.pattern) {
        throw std::runtime_error("Macroblock type unhandled.");
    }

    return block;
}

namespace {
    int sign(int in) {
        if (in > 0) return 1;
        else if (in == 0) return 0;
        else return -1;
    }
}

std::array<image::Colour, 256> mpeg1::read_intra_blocks(util::bitspan& data, BlockContext& context) {
    using namespace image;

    std::array<Colour, 256> block;
    for (size_t block_i = 0; block_i < 6; block_i++) {
        std::array<int, 64> dct_recon;
        std::fill(dct_recon.begin(), dct_recon.end(), 0);
        int dct_dc_size = 0;
        int* dct_dc_past = &context.dct_dc_y_past;

        if (block_i < 4) {
            // dct_dc_size_luminance
            dct_dc_size = mpeg1::BLOCK_DCT_DC_SIZE_LUMINANCE.next_symbol(data);
        } else {
            // dct_dc_size_chrominance
            dct_dc_size = mpeg1::BLOCK_DCT_DC_SIZE_CHROMINANCE.next_symbol(data);

            if (block_i == 4) {
                dct_dc_past = &context.dct_dc_cb_past;
            } else if (block_i == 5) {
                dct_dc_past = &context.dct_dc_cr_past;
            }
        }

        if (dct_dc_size > 0) {
            size_t dct_dc_differential = data.read_bits_be(dct_dc_size);
            dct_recon[0] = calc_dct_zz_zero(dct_dc_size, dct_dc_differential);
        }
        dct_recon[0] *= 8;
        if ((context.macroblock_address - context.past_intra_address > 1) &&
            (block_i == 0 || block_i == 4 || block_i == 5)) {
            dct_recon[0] = (128 * 8) + dct_recon[0];
        } else {
            dct_recon[0] = *dct_dc_past + dct_recon[0];
        }
        *dct_dc_past = dct_recon[0];

        size_t dct_i = 0;
        while (data.peek_bits_be(2) != 0b10) {
            DCTCoeff next;

            if (data.peek_bits_be(6) == 0b000001) {
                //escape code
                data.read_bits_be(6);
                next.run = data.read_bits_be(6);

                next.level = data.read_bits_be(8);
                if (next.level == 0x80 || next.level == 0x00) {
                    next.level <<= 8;
                    next.level |= data.read_bits_be(8);
                } else if (next.level > 127) {
                    next.level -= 256;
                }
            } else {
                next = mpeg1::BLOCK_DCT_COEFF.next_symbol(data);
                auto sign = data.read_bits_be(1);
                if (sign == 1) {
                    next.level *= -1;
                }
            }
            
            dct_i += next.run + 1;
            auto index = mpeg1::ZIGZAG_INDEX[dct_i];
            dct_recon[index] = (2 * next.level * context.slice.quantizer_scale * context.sequence.intra_quantizer_matrix[index]) / 16;

            if ((dct_recon[index] & 1) == 0) {
                dct_recon[index] -= sign(dct_recon[index]);
            }

            if (dct_recon[index] > 2047) {
                dct_recon[index] = 2047;
            } else if (dct_recon[index] < -2048) {
                dct_recon[index] = -2048;
            }
        }

        data.read_bits_be(2);

        image::idct(dct_recon);
        for (auto& val : dct_recon) {
            val = std::max(0, std::min(val, 255));
        }

        if (block_i < 4) {
            //y
            auto start = 0;
            if (block_i == 1 || block_i == 3) {
                start += 8;
            }
            if (block_i == 2 || block_i == 3) {
                start += 8 * 16; // 8 lines of 16 subpixels
            }

            auto apply = [](const int in, Colour& out) { out.y = in; };

            util::transform_out(dct_recon.begin(), dct_recon.begin() + 8, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 8, dct_recon.begin() + 16, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 16, dct_recon.begin() + 24, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 24, dct_recon.begin() + 32, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 32, dct_recon.begin() + 40, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 40, dct_recon.begin() + 48, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 48, dct_recon.begin() + 56, block.begin() + start, apply);
            start += 16;
            util::transform_out(dct_recon.begin() + 56, dct_recon.begin() + 64, block.begin() + start, apply);
        } else if (block_i == 4) {
            //cb
            for (size_t y = 0; y < 8; y++) {
                for (size_t x = 0; x < 8; x++) {
                    auto val = dct_recon[x + y * 8];

                    block[x * 2 + y * 2 * 16].cb = val;
                    block[x * 2 + 1 + y * 2 * 16].cb = val;
                    block[x * 2 + y * 2 * 16 + 16].cb = val;
                    block[x * 2 + 1 + y * 2 * 16 + 16].cb = val;
                }
            }
        } else if (block_i == 5) {
            //cr
            for (size_t y = 0; y < 8; y++) {
                for (size_t x = 0; x < 8; x++) {
                    auto val = dct_recon[x + y * 8];

                    block[x * 2 + y * 2 * 16].cr = val;
                    block[x * 2 + 1 + y * 2 * 16].cr = val;
                    block[x * 2 + y * 2 * 16 + 16].cr = val;
                    block[x * 2 + 1 + y * 2 * 16 + 16].cr = val;
                }
            }
        }
    }

    context.past_intra_address = context.macroblock_address;

    for (auto& c : block) {
        c = ycbcrToRGB(c);
    }

    return block;
}

size_t mpeg1::calc_dct_zz_zero(size_t dc_size, size_t dc_differential) {
    if (dc_differential & (1 << (dc_size - 1))) {
        return dc_differential;
    } else {
        return ((-1) << (dc_size)) | (dc_differential + 1);
    }
}
