
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

    int mb_luma_to_block(int mb_addr, uint32_t block_num, const image::PixelBuffer& destination) {
        size_t span_mb = destination.width() / 16;
        size_t addrHorizontal = mb_addr % span_mb;
        size_t addrVertical = mb_addr / span_mb;

        addrHorizontal *= 2;
        addrVertical *= 2;

        if (block_num == 1 || block_num == 3) {
            addrHorizontal += 1;
        }

        if (block_num == 2 || block_num == 3) {
            addrVertical += 1;
        }

        return addrVertical * destination.span_mb() + addrHorizontal;
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
    int previous_macroblock_address = (_slice.vertical_position - 1) * _sequence.mb_width() - 1;

    try {
        while (!peak_code(bits)) {
            auto macroblock = mpeg1::read_macroblock(bits, _picture);
            int macroblock_address = previous_macroblock_address + macroblock.address_increment;

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
                read_intra_blocks(bits, macroblock_address, destination);
            } else {
                auto mv = mpeg1::calc_motion_vectors(_picture, macroblock, std::make_tuple(_mv_right_for_prev, _mv_down_for_prev));
                std::tie(_mv_right_for_prev, _mv_down_for_prev) = mv;
                copy_mv_block(macroblock_address, mv, source, destination);
                
                if (macroblock.type.pattern) {
                    for (size_t i = 0; i < 6; i++) {
                        if (!mpeg1::check_cbp(macroblock.coded_block_pattern, i)) {
                            continue;
                        }

                        auto dct = read_block(bits, i);

                        if (i < 4) {
                            auto blockAddr = mb_luma_to_block(macroblock_address, i, destination.luma);
                            add_block_to_buff(blockAddr, dct, destination.luma);
                        } else if (i == 4) {
                            add_block_to_buff(macroblock_address, dct, destination.cb);
                        } else if (i == 5) {
                            add_block_to_buff(macroblock_address, dct, destination.cr);
                        }
                    }
                }
            }

            previous_macroblock_address = macroblock_address;
        }
    } catch (std::exception& e) {
        std::println("Experienced exception: {}", e.what());
    }

    data = data.subspan(bits.bytes_read());

    return previous_macroblock_address;
}

void mpeg1::SliceDecoder::reset() {
    _dct_dc_y_past = 1024;
    _dct_dc_cb_past = 1024;
    _dct_dc_cr_past = 1024;
    _past_intra_address = -2;
    _mv_right_for_prev = 0;
    _mv_down_for_prev = 0;
}

void mpeg1::SliceDecoder::read_intra_blocks(util::bitspan& data,
                                            int mb_addr,
                                            image::Frame& destination) {
    for (size_t block_i = 0; block_i < 6; block_i++) {
        auto dct_recon = read_intra_block(data, mb_addr, block_i);

        if (block_i < 4) {
            auto blockAddr = mb_luma_to_block(mb_addr, block_i, destination.luma);
            copy_block_to_buff(blockAddr, dct_recon, destination.luma);
        } else if (block_i == 4) {
            copy_block_to_buff(mb_addr, dct_recon, destination.cb);
        } else if (block_i == 5) {
            copy_block_to_buff(mb_addr, dct_recon, destination.cr);
        }
    }

    _past_intra_address = mb_addr;
}

std::array<int32_t, 8*8> mpeg1::SliceDecoder::read_intra_block(util::bitspan& data, int mb_addr, int block_num) {
    std::array<int, 64> dct_recon{};
    int dct_dc_size = 0;
    int* dct_dc_past = &_dct_dc_y_past;

    if (block_num < 4) {
        // dct_dc_size_luminance
        dct_dc_size = mpeg1::BLOCK_DCT_DC_SIZE_LUMINANCE.next_symbol(data);
    } else {
        // dct_dc_size_chrominance
        dct_dc_size = mpeg1::BLOCK_DCT_DC_SIZE_CHROMINANCE.next_symbol(data);

        if (block_num == 4) {
            dct_dc_past = &_dct_dc_cb_past;
        } else if (block_num == 5) {
            dct_dc_past = &_dct_dc_cr_past;
        }
    }

    if (dct_dc_size > 0) {
        size_t dct_dc_differential = data.read_bits_be(dct_dc_size);
        dct_recon[0] = calc_dct_zz_zero(dct_dc_size, dct_dc_differential);
    }
    dct_recon[0] *= 8;
    if ((mb_addr - _past_intra_address > 1) &&
        (block_num == 0 || block_num == 4 || block_num == 5)) {
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

    return dct_recon;
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
