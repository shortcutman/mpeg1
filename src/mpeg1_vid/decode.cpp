
//------------------------------------------------------------------------------
// decode.cpp
//------------------------------------------------------------------------------

#include "decode.hpp"

#include "mpeg1.hpp"
#include "mpeg1.consts.hpp"

#include <cstddef>
#include <print>
#include <span>

namespace {
    void copy_mb_to_image(int addr, const std::array<image::Colour, 256>& block, std::vector<image::Colour>& image) {
        const size_t span = 640;

        // addr -= 1; //addr starts from 1
        auto addrHorizontal = addr % 40;
        auto addrVertical = addr / 40;
        auto imageStartIt = image.begin() + addrHorizontal * 16 + addrVertical * 16 * span;
        for (size_t i = 0; i < 16; i++) {
            auto blockIt = block.begin() + i * 16;
            auto imageIt = imageStartIt + i * span;
            std::copy(blockIt, blockIt + 16, imageIt);
        }
    }

    std::array<image::Colour, 256> copy_block_mv_from_image(int addr, std::tuple<int, int> motion_vector, const std::vector<image::Colour>& source) {
        const size_t span = 640;
        std::array<image::Colour, 256> block;

        auto right_for = std::get<0>(motion_vector) >> 1;
        auto down_for = std::get<1>(motion_vector) >> 1;
        auto right_half_for = std::get<0>(motion_vector) - 2 * right_for;
        auto down_half_for = std::get<1>(motion_vector) - 2 * down_for;

        auto addrHorizontal = addr % (span / 16);
        auto addrVertical = addr / (span / 16);

        //implement copy of pels as per bottom of page 35
        if (!right_half_for && !down_half_for) {
            auto sourceStartIt = source.begin() + addrHorizontal * 16 + down_for + addrVertical * 16 * span + right_for;
            for (size_t i = 0; i < 16; i++) {
                auto sourceIt = sourceStartIt + i * span;
                std::copy(sourceIt, sourceIt + 16, block.begin() + i * 16);
            }
        } else {
            throw std::runtime_error("Unimplemented motion vector copy");
        }

        return block;
    }
}

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

                context.last_predictive.resize(context.sequence.horizontal_size * context.sequence.vertical_size);
                context.current_image.resize(context.sequence.horizontal_size * context.sequence.vertical_size);
            } else if (*code == mpeg1::start_code::group_of_pictures) {
                std::println("\t\tFound GOP header code at byte no. {}", i);
                mpeg1::read_gop_header(bytes);
                i += bytes_size - bytes.size() - 1;
            } else if (*code == mpeg1::start_code::picture) {
                if (pictures != 0) {
                    image::writeOutPPM(std::format("/tmp/danpg1/img_{:04d}.ppm", pictures),
                                    context.sequence.horizontal_size,
                                    context.sequence.vertical_size,
                                    context.current_image);

                    std::copy(context.current_image.begin(), context.current_image.end(), context.last_predictive.begin());
                }

                pictures++;

                context.picture = mpeg1::read_picture_header(bytes);
                i += bytes_size - bytes.size() - 1;

                std::println("\t\t\tFound picture header code at byte no. {}, pic num {} type {}", i, pictures, mpeg1::ct_to_string(context.picture.coding_type));
            } else if (*code >= mpeg1::start_code::slice_minimum &&
                        *code <= mpeg1::start_code::slice_maximum) {

                util::bitspan bits(bytes);
                context.slice = mpeg1::read_slice_header(bits);
                context.previous_macroblock_address = (context.slice.vertical_position - 1) * context.mb_width() - 1;
                context.past_intra_address = -2;
                context.dct_dc_y_past = 1024;
                context.dct_dc_cb_past = 1024;
                context.dct_dc_cr_past = 1024;

                while (!peak_code(bits)) {
                    context.macroblock = mpeg1::read_macroblock(bits, context.picture);
                    context.macroblock_address = context.previous_macroblock_address + context.macroblock.address_increment;

                    if (context.macroblock.type.intra) {
                        auto block = mpeg1::read_intra_blocks(bits, context);
                        copy_mb_to_image(context.macroblock_address, block, context.current_image);
                    } else {
                        auto mv = mpeg1::calc_motion_vectors(context.picture, context.macroblock);
                        auto block = copy_block_mv_from_image(context.macroblock_address, mv, context.last_predictive);
                        copy_mb_to_image(context.macroblock_address, block, context.current_image);
                    }

                    context.previous_macroblock_address = context.macroblock_address;
                }
            }
        }
    }
}