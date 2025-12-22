
//------------------------------------------------------------------------------
// slicedecoder.cpp
//------------------------------------------------------------------------------

#include "slicedecoder.hpp"

#include "copy.hpp"
#include "idct.hpp"
#include "mpeg1.hpp"
#include "vlctables.hpp"

namespace {
    bool peak_code(const util::bitspan& bits) {
        auto unread_bytes_span = bits.to_aligned_span();
        return (unread_bytes_span[0] == std::byte{0x00} &&
                unread_bytes_span[1] == std::byte{0x00} &&
                unread_bytes_span[2] == std::byte{0x01});
    }

    int sign(int in) {
        if (in > 0) return 1;
        else if (in == 0) return 0;
        else return -1;
    }
}

mpeg1::SliceDecoder::SliceDecoder(mpeg1::SequenceHeader sequence, mpeg1::PictureHeader picture)
: _sequence(sequence)
, _picture(picture) {
    reset();
}

int mpeg1::SliceDecoder::decode(std::span<std::byte>& data, const image::Frame& source, image::Frame& destination) {
    util::bitspan bits(data);
    reset();

    _slice = mpeg1::read_slice_header(bits);
    _previous_macroblock_address = (_slice.vertical_position - 1) * _sequence.mb_width() - 1;

    try {
        while (!peak_code(bits)) {
            auto macroblock = mpeg1::read_macroblock(bits, _picture);
            _macroblock_address = _previous_macroblock_address + macroblock.address_increment;

            if (!macroblock.type.motion_forward || macroblock.address_increment > 1) {
                _mv_right_for_prev = 0;
                _mv_down_for_prev = 0;
            }

            if (!macroblock.type.intra || macroblock.address_increment > 1) {
                _dct_dc_y_past = 1024;
                _dct_dc_cb_past = 1024;
                _dct_dc_cr_past = 1024;
            }

            if (macroblock.type.intra) {
                auto block = read_intra_blocks(bits);
                copy_mb_to_image(_macroblock_address, block, destination);
            } else {
                auto mv = mpeg1::calc_motion_vectors(_picture, macroblock, std::make_tuple(_mv_right_for_prev, _mv_down_for_prev));
                std::tie(_mv_right_for_prev, _mv_down_for_prev) = mv;
                auto block = copy_block_mv_from_image(_macroblock_address, mv, source);
                
                if (macroblock.type.pattern) {
                    for (size_t i = 0; i < 6; i++) {
                        if (!mpeg1::check_cbp(macroblock.coded_block_pattern, i)) {
                            continue;
                        }

                        auto dct = read_block(bits, i);

                        if (i < 4) {
                            assign_to_y(dct, block, i);
                        } else if (i == 4) {
                            assign_to_cb(dct, block);
                        } else if (i == 5) {
                            assign_to_cr(dct, block);
                        }
                    }
                }

                copy_mb_to_image(_macroblock_address, block, destination);
            }

            _previous_macroblock_address = _macroblock_address;
        }
    } catch (std::exception& e) {
        std::println("Experienced exception: {}", e.what());
    }

    data = data.subspan(bits.bytes_read());

    return _macroblock_address;
}

void mpeg1::SliceDecoder::reset() {
    _dct_dc_y_past = 1024;
    _dct_dc_cb_past = 1024;
    _dct_dc_cr_past = 1024;
    _past_intra_address = -2;
    _mv_right_for_prev = 0;
    _mv_down_for_prev = 0;
}

std::array<image::Colour, 256> mpeg1::SliceDecoder::read_intra_blocks(util::bitspan& data) {
    std::array<image::Colour, 256> block;
    for (size_t block_i = 0; block_i < 6; block_i++) {
        std::array<int, 64> dct_recon;
        std::fill(dct_recon.begin(), dct_recon.end(), 0);
        int dct_dc_size = 0;
        int* dct_dc_past = &_dct_dc_y_past;

        if (block_i < 4) {
            // dct_dc_size_luminance
            dct_dc_size = mpeg1::BLOCK_DCT_DC_SIZE_LUMINANCE.next_symbol(data);
        } else {
            // dct_dc_size_chrominance
            dct_dc_size = mpeg1::BLOCK_DCT_DC_SIZE_CHROMINANCE.next_symbol(data);

            if (block_i == 4) {
                dct_dc_past = &_dct_dc_cb_past;
            } else if (block_i == 5) {
                dct_dc_past = &_dct_dc_cr_past;
            }
        }

        if (dct_dc_size > 0) {
            size_t dct_dc_differential = data.read_bits_be(dct_dc_size);
            dct_recon[0] = calc_dct_zz_zero(dct_dc_size, dct_dc_differential);
        }
        dct_recon[0] *= 8;
        if ((_macroblock_address - _past_intra_address > 1) &&
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
            dct_recon[index] = (2 * next.level * _slice.quantizer_scale * _sequence.intra_quantizer_matrix[index]) / 16;

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
            assign_to_y(dct_recon, block, block_i);
        } else if (block_i == 4) {
            assign_to_cb(dct_recon, block);
        } else if (block_i == 5) {
            assign_to_cr(dct_recon, block);
        }
    }

    _past_intra_address = _macroblock_address;

    return block;
}

std::array<int, 64> mpeg1::SliceDecoder::read_block(util::bitspan& data, size_t block_index) {
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

        dct_recon[index] = ((2 * next.level + sign(next.level)) * _slice.quantizer_scale * _sequence.non_intra_quantizer_matrix[index]) / 16;

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

    image::idct(dct_recon);
    for (auto& val : dct_recon) {
        val = std::max(-256, std::min(val, 255));
    }

    return dct_recon;
}

void mpeg1::SliceDecoder::set_slice(mpeg1::SliceHeader slice) {
    _slice = slice;
}
