
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

mpeg1::Macroblock mpeg1::read_macroblock(util::bitspan& data, const PictureHeader& picture) {
    Macroblock block;

    while (data.peek_bits_be(11) == 0x00f) {
        data.read_bits_be(11);
    }

    while (data.peek_bits_be(11) == 0x008) {
        data.read_bits_be(11);
        block.address_increment += 33;
    }

    block.address_increment = mpeg1::MACROBLOCK_ADDRESSING.next_symbol(data);

    switch (picture.coding_type) {
        case CodingType::IntraCoded: {
            block.type = mpeg1::MACROBLOCK_TYPE_INTRA_VLC.next_symbol(data);
        }
        break;

        case CodingType::PredictiveCoded: {
            block.type = mpeg1::MACROBLOCK_TYPE_PRED_VLC.next_symbol(data);
        }
        break;

        default: {
            throw std::runtime_error("Unhandled CodingType for macroblock.");
        }
        break;
    }

    if (block.type.quant) {
        block.quantizer_scale = data.read_bits_be(5);
    }

    if (block.type.motion_forward) {
        auto forward_r_size = picture.forward_f_code - 1;
        auto forward_f = 1 << forward_r_size;

        block.motion_horizontal_forward_code = mpeg1::MACROBLOCK_MOTION_VECTOR_CODES.next_symbol(data);
        if (forward_f != 1 && block.motion_horizontal_forward_code != 0) {
            block.motion_horizontal_forward_r = data.read_bits_be(forward_r_size);
        }

        block.motion_vertical_forward_code = mpeg1::MACROBLOCK_MOTION_VECTOR_CODES.next_symbol(data);
        if (forward_f != 1 && block.motion_vertical_forward_code != 0) {
            block.motion_vertical_forward_r = data.read_bits_be(forward_r_size);
        }
    }

    if (block.type.motion_backward) {
        auto backward_r_size = picture.backward_f_code - 1;
        auto backward_f = 1 << backward_r_size;

        block.motion_horizontal_backward_code = mpeg1::MACROBLOCK_MOTION_VECTOR_CODES.next_symbol(data);
        if (backward_f != 1 && block.motion_horizontal_backward_code != 0) {
            block.motion_horizontal_backward_r = data.read_bits_be(backward_r_size);
        }

        block.motion_vertical_backward_code = mpeg1::MACROBLOCK_MOTION_VECTOR_CODES.next_symbol(data);
        if (backward_f != 1 && block.motion_vertical_backward_code != 0) {
            block.motion_vertical_backward_r = data.read_bits_be(backward_r_size);
        }
    }

    if (block.type.pattern) {
        block.coded_block_pattern = mpeg1::MACROBLOCK_CODED_BLOCK_PATTERN.next_symbol(data);
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
                next = mpeg1::BLOCK_DCT_COEFF_NEXT.next_symbol(data);
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

std::array<int, 64> mpeg1::read_block(util::bitspan& data, BlockContext& context, size_t block_index) {    
    std::array<int, 64> dct_recon;
    std::fill(dct_recon.begin(), dct_recon.end(), 0);

    size_t dct_i = 0;
    do {
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
            if (dct_i == 0) {
                next = mpeg1::BLOCK_DCT_COEFF_FIRST.next_symbol(data);
            } else {
                next = mpeg1::BLOCK_DCT_COEFF_NEXT.next_symbol(data);
            }
            
            auto sign = data.read_bits_be(1);
            if (sign == 1) {
                next.level *= -1;
            }
        }
        
        dct_i += next.run;
        auto index = mpeg1::ZIGZAG_INDEX[dct_i];
        dct_i++;

        dct_recon[index] = (2 * next.level * context.slice.quantizer_scale * context.sequence.non_intra_quantizer_matrix[index]) / 16;

        if ((dct_recon[index] & 1) == 0) {
            dct_recon[index] -= sign(dct_recon[index]);
        }

        if (dct_recon[index] > 2047) {
            dct_recon[index] = 2047;
        } else if (dct_recon[index] < -2048) {
            dct_recon[index] = -2048;
        }
    } while (data.peek_bits_be(2) != 0b10);

    data.read_bits_be(2);

    return dct_recon;
}

std::tuple<int, int> mpeg1::calc_motion_vectors(const PictureHeader& picture, const Macroblock& macroblock) {
    static auto recon_right_for_prev = 0;
    static auto recon_down_for_prev = 0;

    auto forward_r_size = picture.forward_f_code - 1;
    auto forward_f = 1 << forward_r_size;

    auto complement_horizontal_forward_r = 0;
    auto complement_vertical_forward_r = 0;

    if (forward_f == 1 || macroblock.motion_horizontal_forward_code == 0) {
        complement_horizontal_forward_r = 0;
    } else {
        complement_horizontal_forward_r = forward_f - 1 - macroblock.motion_horizontal_forward_r;
    }

    if (forward_f == 1 || macroblock.motion_vertical_forward_code == 0) {
        complement_vertical_forward_r = 0;
    } else {
        complement_vertical_forward_r = forward_f - 1 - macroblock.motion_vertical_forward_r;
    }

    auto right_little = macroblock.motion_horizontal_forward_code * forward_f;
    auto right_big = 0;
    if (right_little == 0) {
        right_big = 0;
    } else {
        if (right_little > 0) {
            right_little = right_little - complement_horizontal_forward_r;
            right_big = right_little - (32 * forward_f);
        } else {
            right_little = right_little + complement_horizontal_forward_r;
            right_big = right_little + (32 * forward_f);
        }
    }

    auto down_little = macroblock.motion_vertical_forward_code * forward_f;
    auto down_big = 0;
    if (down_little == 0) {
        down_big = 0;
    } else {
        if (down_little > 0) {
            down_little = down_little - complement_vertical_forward_r;
            down_big = down_little - (32 * forward_f);
        } else {
            down_little = down_little + complement_vertical_forward_r;
            down_big = down_little + (32 * forward_f);
        }
    }

    auto max = (16 * forward_f) - 1;
    auto min = -16 * forward_f;

    auto new_vector = recon_right_for_prev + right_little;
    auto recon_right_for = 0;
    if (new_vector <= max && new_vector >= min) {
        recon_right_for = recon_right_for_prev + right_little;
    } else {
        recon_right_for = recon_right_for_prev + right_big;
    }
    recon_right_for_prev = recon_right_for;

    if (picture.full_pel_forward_vector) {
        recon_right_for = recon_right_for << 1;
    }

    new_vector = recon_down_for_prev + down_little;
    auto recon_down_for = 0;
    if (new_vector <= max && new_vector >= min) {
        recon_down_for = recon_down_for_prev + down_little;
    } else {
        recon_down_for = recon_down_for_prev + down_big;
    }
    recon_down_for_prev = recon_down_for;
    if (picture.full_pel_forward_vector) {
        recon_right_for = recon_right_for << 1;
    }

    return std::make_tuple(recon_right_for, recon_down_for);
}

size_t mpeg1::calc_dct_zz_zero(size_t dc_size, size_t dc_differential) {
    if (dc_differential & (1 << (dc_size - 1))) {
        return dc_differential;
    } else {
        return ((-1) << (dc_size)) | (dc_differential + 1);
    }
}

bool mpeg1::check_cbp(uint32_t coded_block_pattern, size_t index) {
    return coded_block_pattern & (1 << (5 - index));
}
