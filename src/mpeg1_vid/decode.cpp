
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
    mpeg1::BlockContext context;

    for (size_t i = 0; i < data.size(); i++) {
        auto bytes = std::span<std::byte>(data.begin() + i, data.end());
        auto code = get_code(bytes);
        if (code) {
            const auto bytes_size = bytes.size();

            if (*code == mpeg1::start_code::sequence) {
                std::println("\tFound sequence header code at byte no. {}", i);
                context.sequence = mpeg1::read_sequence_header(bytes);
                i += bytes_size - bytes.size() - 1;

                context.last_predictive.encoded_width = context.sequence.encoded_width();
                context.last_predictive.encoded_height = context.sequence.encoded_height();
                context.last_predictive.image.resize(context.sequence.encoded_width() * context.sequence.encoded_height());

                context.current_image.encoded_width = context.sequence.encoded_width();
                context.current_image.encoded_height = context.sequence.encoded_height();
                context.current_image.image.resize(context.sequence.encoded_width() * context.sequence.encoded_height());
            } else if (*code == mpeg1::start_code::group_of_pictures) {
                std::println("\t\tFound GOP header code at byte no. {}", i);
                mpeg1::read_gop_header(bytes);
                i += bytes_size - bytes.size() - 1;
            } else if (*code == mpeg1::start_code::picture) {
                if (pictures != 0) {
                    auto imagecopy = context.current_image;

                    for (auto&c : imagecopy.image) {
                        c = image::ycbcrToRGB(c);
                    }

                    image::writeOutPPM(std::format("/tmp/danpg1/img_{:04d}_{}.ppm", pictures, mpeg1::ct_to_string(context.picture.coding_type)),
                                    imagecopy.encoded_width,
                                    imagecopy.encoded_height,
                                    imagecopy.image);

                    std::copy(context.current_image.image.begin(), context.current_image.image.end(), context.last_predictive.image.begin());
                }

                pictures++;

                context.picture = mpeg1::read_picture_header(bytes);
                i += bytes_size - bytes.size() - 1;

                std::println("\t\t\tFound picture header code at byte no. {}, pic num {} type {}", i, pictures, mpeg1::ct_to_string(context.picture.coding_type));
            } else if (*code >= mpeg1::start_code::slice_minimum &&
                        *code <= mpeg1::start_code::slice_maximum) {

                util::bitspan bits(bytes);
                context.slice = mpeg1::read_slice_header(bits);
                context.previous_macroblock_address = (context.slice.vertical_position - 1) * context.sequence.mb_width() - 1;
                context.past_intra_address = -2;
                context.dct_dc_y_past = 1024;
                context.dct_dc_cb_past = 1024;
                context.dct_dc_cr_past = 1024;
                context.mv_right_for_prev = 0;
                context.mv_down_for_prev = 0;

                while (bits.bits_remaining() > 32 && !peak_code(bits)) {
                    auto macroblock = mpeg1::read_macroblock(bits, context.picture);
                    context.macroblock_address = context.previous_macroblock_address + macroblock.address_increment;

                    if (!macroblock.type.motion_forward || macroblock.address_increment > 1) {
                        context.mv_right_for_prev = 0;
                        context.mv_down_for_prev = 0;
                    }

                    if (macroblock.type.intra) {
                        auto block = mpeg1::read_intra_blocks(bits, context);
                        copy_mb_to_image(context.macroblock_address, block, context.current_image);
                    } else {
                        auto mv = mpeg1::calc_motion_vectors(context.picture, macroblock, std::make_tuple(context.mv_right_for_prev, context.mv_down_for_prev));
                        std::tie(context.mv_right_for_prev, context.mv_down_for_prev) = mv;
                        auto block = copy_block_mv_from_image(context.macroblock_address, mv, context.last_predictive);
                        
                        if (macroblock.type.pattern) {
                            for (size_t i = 0; i < 6; i++) {
                                if (!mpeg1::check_cbp(macroblock.coded_block_pattern, i)) {
                                    continue;
                                }

                                auto dct = mpeg1::read_block(bits, context, i);

                                if (i < 4) {
                                    assign_to_y(dct, block, i);
                                } else if (i == 4) {
                                    assign_to_cb(dct, block);
                                } else if (i == 5) {
                                    assign_to_cr(dct, block);
                                }
                            }
                        }

                        copy_mb_to_image(context.macroblock_address, block, context.current_image);
                    }

                    context.previous_macroblock_address = context.macroblock_address;
                }
            }
        }
    }

    for (auto&c : context.current_image.image) {
        c = image::ycbcrToRGB(c);
    }

    //this could be problematic, no check we got a complete image at end
    image::writeOutPPM(std::format("/tmp/danpg1/img_{:04d}_{}.ppm", pictures, mpeg1::ct_to_string(context.picture.coding_type)),
                        context.current_image.encoded_width,
                        context.current_image.encoded_height,
                        context.current_image.image);
}