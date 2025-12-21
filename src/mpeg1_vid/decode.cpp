
//------------------------------------------------------------------------------
// decode.cpp
//------------------------------------------------------------------------------

#include "decode.hpp"

#include "mpeg1.hpp"
#include "constants.hpp"
#include "colour.hpp"
#include "copy.hpp"

#include <cstddef>
#include <print>
#include <span>

std::optional<uint32_t> mpeg1::get_code(const std::span<std::byte>& data) {
    if (data[0] == std::byte{0x00} &&
        data[1] == std::byte{0x00} &&
        data[2] == std::byte{0x01}) {
            uint32_t code = 0;
            code |= 0x0100 | std::to_integer<uint8_t>(data[3]);
            return code;
    } else {
        return std::nullopt;
    }
}

bool mpeg1::peak_code(const util::bitspan& data) {
    auto unread_bytes_span = data.to_aligned_span();
    return (unread_bytes_span[0] == std::byte{0x00} &&
            unread_bytes_span[1] == std::byte{0x00} &&
            unread_bytes_span[2] == std::byte{0x01});
}

void mpeg1::decode(std::vector<std::byte>& data) {
    std::println("Start video es decoding...");

    size_t pictures = 0;
    mpeg1::PictureHeader picture;
    image::Frame last_predictive;
    image::Frame current_image;
    mpeg1::BlockContext block_context;

    for (size_t i = 0; i < data.size(); i++) {
        auto bytes = std::span<std::byte>(data.begin() + i, data.end());
        auto code = get_code(bytes);
        if (code) {
            const auto bytes_size = bytes.size();

            if (*code == mpeg1::start_code::sequence) {
                std::println("\tFound sequence header code at byte no. {}", i);
                block_context.sequence = mpeg1::read_sequence_header(bytes);
                i += bytes_size - bytes.size() - 1;

                last_predictive.encoded_width = block_context.sequence.encoded_width();
                last_predictive.encoded_height = block_context.sequence.encoded_height();
                last_predictive.image.resize(block_context.sequence.encoded_width() * block_context.sequence.encoded_height());

                current_image.encoded_width = block_context.sequence.encoded_width();
                current_image.encoded_height = block_context.sequence.encoded_height();
                current_image.image.resize(block_context.sequence.encoded_width() * block_context.sequence.encoded_height());
            } else if (*code == mpeg1::start_code::group_of_pictures) {
                std::println("\t\tFound GOP header code at byte no. {}", i);
                mpeg1::read_gop_header(bytes);
                i += bytes_size - bytes.size() - 1;
            } else if (*code == mpeg1::start_code::picture) {
                if (pictures != 0) {
                    auto imagecopy = current_image;

                    for (auto&c : imagecopy.image) {
                        c = image::ycbcrToRGB(c);
                    }

                    image::writeOutPPM(std::format("/tmp/danpg1/img_{:04d}_{}.ppm", pictures, mpeg1::ct_to_string(picture.coding_type)),
                                    imagecopy.encoded_width,
                                    imagecopy.encoded_height,
                                    imagecopy.image);

                    std::copy(current_image.image.begin(), current_image.image.end(), last_predictive.image.begin());
                }

                pictures++;

                picture = mpeg1::read_picture_header(bytes);
                i += bytes_size - bytes.size() - 1;

                std::println("\t\t\tFound picture header code at byte no. {}, pic num {} type {}", i, pictures, mpeg1::ct_to_string(picture.coding_type));
            } else if (*code >= mpeg1::start_code::slice_minimum &&
                        *code <= mpeg1::start_code::slice_maximum) {

                util::bitspan bits(bytes);
                auto slice = mpeg1::read_slice_header(bits);
                block_context = mpeg1::BlockContext{
                    .sequence = block_context.sequence,
                    .slice = slice,
                    .previous_macroblock_address = static_cast<int>((slice.vertical_position - 1) * block_context.sequence.mb_width() - 1),
                    .past_intra_address = -2,
                    .dct_dc_y_past = 1024,
                    .dct_dc_cb_past = 1024,
                    .dct_dc_cr_past = 1024,
                    .mv_right_for_prev = 0,
                    .mv_down_for_prev = 0
                };


                while (bits.bits_remaining() > 32 && !peak_code(bits)) {
                    auto macroblock = mpeg1::read_macroblock(bits, picture);
                    block_context.macroblock_address = block_context.previous_macroblock_address + macroblock.address_increment;

                    if (!macroblock.type.motion_forward || macroblock.address_increment > 1) {
                        block_context.mv_right_for_prev = 0;
                        block_context.mv_down_for_prev = 0;
                    }

                    if (macroblock.type.intra) {
                        auto block = mpeg1::read_intra_blocks(bits, block_context);
                        copy_mb_to_image(block_context.macroblock_address, block, current_image);
                    } else {
                        auto mv = mpeg1::calc_motion_vectors (picture, macroblock, std::make_tuple (block_context.mv_right_for_prev, block_context.mv_down_for_prev));
                        std::tie (block_context.mv_right_for_prev, block_context.mv_down_for_prev) = mv;
                        auto block = copy_block_mv_from_image (block_context.macroblock_address, mv, last_predictive);
                        
                        if (macroblock.type.pattern) {
                            for (size_t i = 0; i < 6; i++) {
                                if (!mpeg1::check_cbp(macroblock.coded_block_pattern, i)) {
                                    continue;
                                }

                                auto dct = mpeg1::read_block(bits, block_context, i);

                                if (i < 4) {
                                    assign_to_y(dct, block, i);
                                } else if (i == 4) {
                                    assign_to_cb(dct, block);
                                } else if (i == 5) {
                                    assign_to_cr(dct, block);
                                }
                            }
                        }

                        copy_mb_to_image (block_context.macroblock_address, block, current_image);
                    }

                    block_context.previous_macroblock_address = block_context.macroblock_address;
                }
            }
        }
    }

    for (auto&c : current_image.image) {
        c = image::ycbcrToRGB(c);
    }

    //this could be problematic, no check we got a complete image at end
    image::writeOutPPM(std::format("/tmp/danpg1/img_{:04d}_{}.ppm", pictures, mpeg1::ct_to_string (picture.coding_type)),
                        current_image.encoded_width,
                        current_image.encoded_height,
                        current_image.image);
}