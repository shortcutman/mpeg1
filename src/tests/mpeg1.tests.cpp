
//------------------------------------------------------------------------------
// mpeg1.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1.hpp"

#include <gtest/gtest.h>

namespace {

// https://stackoverflow.com/a/45172360
template<typename... Ts>
std::array<std::byte, sizeof...(Ts)> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

}

TEST(MPEG1, read_sequence_header) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0xb3, 0x3c, 0x02, 0x1c, 0x13, 0xff, 0xff, 0xe0, 0xd0);
    auto span = std::span<std::byte>(data);
    auto header = mpeg1::read_sequence_header(span);

    EXPECT_EQ(header.horizontal_size, 960);
    EXPECT_EQ(header.vertical_size, 540);
    EXPECT_EQ(header.pel_aspect_ratio, 1.0f);
    EXPECT_EQ(header.picture_rate, 25.0f);
    EXPECT_EQ(header.bit_rate, 104857200);
    EXPECT_EQ(header.intra_quantizer_matrix, mpeg1::DEFAULT_INTRA_QUANTIZER_MATRIX);
    EXPECT_EQ(header.non_intra_quantizer_matrix, mpeg1::DEFAULT_NON_INTRA_QUANTIZER_MATRIX);
    EXPECT_TRUE(span.empty());
}

TEST(MPEG1, read_sequence_header_fail_on_start_code) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x00);
    auto span = std::span<std::byte>(data);
    EXPECT_THROW(mpeg1::read_sequence_header(span), std::runtime_error);
}

TEST(MPEG1, calculate_sizes) {
    mpeg1::BlockContext context;
    context.sequence = {
        .horizontal_size = 640,
        .vertical_size = 266
    };

    EXPECT_EQ(context.mb_width(), 40);
    EXPECT_EQ(context.mb_height(), 17);
    EXPECT_EQ(context.encoded_width(), 640);
    EXPECT_EQ(context.encoded_height(), 272);
}

TEST(MPEG1, read_group_of_pictures_header) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0xb8, 0x00, 0x08, 0x00, 0x40);
    auto span = std::span<std::byte>(data);
    auto header = mpeg1::read_gop_header(span);

    EXPECT_EQ(header.closed_gop, true);
    EXPECT_EQ(header.broken_link, false);
    EXPECT_TRUE(span.empty());
}

TEST(MPEG1, read_group_of_pictures_header_fail_on_start_code) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x00);
    auto span = std::span<std::byte>(data);
    EXPECT_THROW(mpeg1::read_gop_header(span), std::runtime_error);
}

TEST(MPEG1, read_picture_header_intracoded_frame) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x00, 0x00, 0x0f, 0xff, 0xf8);
    auto span = std::span<std::byte>(data);
    auto header = mpeg1::read_picture_header(span);

    EXPECT_EQ(header.temporal_reference, 0);
    EXPECT_EQ(header.coding_type, mpeg1::CodingType::IntraCoded);
    EXPECT_TRUE(span.empty());
}

TEST(MPEG1, read_picture_header_fail_on_start_code) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x01, 0x00, 0x0f, 0xff);
    auto span = std::span<std::byte>(data);
    EXPECT_THROW(mpeg1::read_picture_header(span), std::runtime_error);
}

TEST(MPEG1, read_slice_header) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x01, 0x13, 0xf8);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    auto header = mpeg1::read_slice_header(bits);

    EXPECT_EQ(header.vertical_position, 1);
    EXPECT_EQ(header.quantizer_scale, 2);
    EXPECT_EQ(bits.bits_read(), 38);
}

TEST(MPEG1, read_slice_header_fail_on_start_code_below_min) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x00);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    EXPECT_THROW(mpeg1::read_slice_header(bits), std::runtime_error);
}

TEST(MPEG1, read_slice_header_fail_on_start_code_above_max) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0xb0);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    EXPECT_THROW(mpeg1::read_slice_header(bits), std::runtime_error);
}

TEST(MPEG1, read_block) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x01, 0x13, 0xf8, 0x00, 0x00);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    mpeg1::read_slice_header(bits); //align test data
    mpeg1::PictureHeader pic_header{ .coding_type = mpeg1::CodingType::IntraCoded };
    auto block = mpeg1::read_macroblock(bits, pic_header);

    EXPECT_EQ(block.address_increment, 1);
    EXPECT_EQ(block.type.quant, false);
    EXPECT_EQ(block.type.motion_forward, false);
    EXPECT_EQ(block.type.motion_backward, false);
    EXPECT_EQ(block.type.pattern, false);
    EXPECT_EQ(block.type.intra, true);
}

TEST(MPEG1, read_intraframe_intra_macroblock_header) {
    auto data = make_bytes(0x23, 0xf8, 0x85);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    bits.read_bits_be(6);

    mpeg1::PictureHeader pic_header{ .coding_type = mpeg1::CodingType::IntraCoded };
    auto macroblock = mpeg1::read_macroblock(bits, pic_header);

    EXPECT_EQ(macroblock.address_increment, 1);
    EXPECT_EQ(macroblock.type.quant, false);
    EXPECT_EQ(macroblock.type.motion_forward, false);
    EXPECT_EQ(macroblock.type.motion_backward, false);
    EXPECT_EQ(macroblock.type.pattern, false);
    EXPECT_EQ(macroblock.type.intra, true);
}

TEST(MPEG1, read_predframe_intra_macroblock_header) {
    auto data = make_bytes(0x22, 0x70, 0x10);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    bits.read_bits_be(6);

    mpeg1::PictureHeader pic_header{
        .coding_type = mpeg1::CodingType::PredictiveCoded,
        .forward_f_code = 1
    };
    auto macroblock = mpeg1::read_macroblock(bits, pic_header);

    EXPECT_EQ(macroblock.address_increment, 1);
    EXPECT_EQ(macroblock.type.quant, false);
    EXPECT_EQ(macroblock.type.motion_forward, true);
    EXPECT_EQ(macroblock.type.motion_backward, false);
    EXPECT_EQ(macroblock.type.pattern, false);
    EXPECT_EQ(macroblock.type.intra, false);
    EXPECT_EQ(macroblock.quantizer_scale, 0);
    EXPECT_EQ(macroblock.motion_horizontal_forward_code, 0);
    EXPECT_EQ(macroblock.motion_horizontal_forward_r, 0);
    EXPECT_EQ(macroblock.motion_vertical_forward_code, 0);
    EXPECT_EQ(macroblock.motion_vertical_forward_r, 0);
    EXPECT_EQ(macroblock.motion_horizontal_backward_code, 0);
    EXPECT_EQ(macroblock.motion_horizontal_backward_r, 0);
    EXPECT_EQ(macroblock.motion_vertical_backward_code, 0);
    EXPECT_EQ(macroblock.motion_vertical_backward_r, 0);
    EXPECT_EQ(macroblock.coded_block_pattern, 0);
}

TEST(MPEG1, read_predictive_macroblock) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x01, 0x23, 0x97, 0x3a, 0x50);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);

    auto pic_header = mpeg1::PictureHeader{
        .coding_type = mpeg1::CodingType::PredictiveCoded,
        .forward_f_code = 2
    };

    auto slice_header = mpeg1::read_slice_header(bits);
    EXPECT_EQ(slice_header.vertical_position, 1);
    EXPECT_EQ(slice_header.quantizer_scale, 4);

    auto mb1 = mpeg1::read_macroblock(bits, pic_header);
    EXPECT_EQ(mb1.address_increment, 1);
    EXPECT_EQ(mb1.type.intra, false);
    EXPECT_EQ(mb1.type.motion_backward, false);
    EXPECT_EQ(mb1.type.motion_forward, true);
    EXPECT_EQ(mb1.type.quant, false);
    EXPECT_EQ(mb1.type.pattern, true);

    EXPECT_EQ(mb1.motion_horizontal_forward_code, 0);
    EXPECT_EQ(mb1.motion_vertical_forward_code, 2);
    EXPECT_EQ(mb1.coded_block_pattern, 8);
}

TEST(MPEG1, read_block_unhandled) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x01, 0x13, 0xf8, 0x00, 0x00);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    mpeg1::read_slice_header(bits); //align test data

    mpeg1::PictureHeader bipred_pic{ .coding_type = mpeg1::CodingType::BidirectionalPredCoded };
    EXPECT_THROW(mpeg1::read_macroblock(bits, bipred_pic), std::runtime_error);
}

TEST(MPEG1, calc_motion_vectors_predictive1) {
    mpeg1::PictureHeader pred_pic{
        .forward_f_code = 0,
        .full_pel_forward_vector = true
    };

    mpeg1::Macroblock block{
        .motion_horizontal_forward_code = 0,
        .motion_horizontal_forward_r = 0,
        .motion_vertical_forward_code = 0,
        .motion_vertical_forward_r = 0
    };

    EXPECT_EQ(mpeg1::calc_motion_vectors(pred_pic, block), std::make_tuple(0, 0));
}

TEST(MPEG1, calc_dct_zz_zero) {
    //From example on page 30 of ISO 11172-2:1993
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b000), -7);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b001), -6);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b010), -5);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b011), -4);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b100), 4);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b101), 5);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b110), 6);
    EXPECT_EQ(mpeg1::calc_dct_zz_zero(3, 0b111), 7);
}

TEST(MPEG1, check_cbp) {
    EXPECT_EQ(mpeg1::check_cbp(60, 0), true);
    EXPECT_EQ(mpeg1::check_cbp(60, 1), true);
    EXPECT_EQ(mpeg1::check_cbp(60, 2), true);
    EXPECT_EQ(mpeg1::check_cbp(60, 3), true);
    EXPECT_EQ(mpeg1::check_cbp(60, 4), false);
    EXPECT_EQ(mpeg1::check_cbp(60, 5), false);

    EXPECT_EQ(mpeg1::check_cbp(3, 0), false);
    EXPECT_EQ(mpeg1::check_cbp(3, 1), false);
    EXPECT_EQ(mpeg1::check_cbp(3, 2), false);
    EXPECT_EQ(mpeg1::check_cbp(3, 3), false);
    EXPECT_EQ(mpeg1::check_cbp(3, 4), true);
    EXPECT_EQ(mpeg1::check_cbp(3, 5), true);
}

TEST(MPEG1, read_intra_macroblock_and_block_data) {
    auto data = make_bytes(
	0x00, 0x00, 0x01, 0x01, 0x23, 0xf8, 0x85, 0x29,
	0x48, 0x8b);

    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    mpeg1::read_slice_header(bits); //align test data
    
    mpeg1::PictureHeader pic_header{ .coding_type = mpeg1::CodingType::IntraCoded };
    mpeg1::read_macroblock(bits, pic_header);
    mpeg1::BlockContext context;
    auto macroblock = mpeg1::read_intra_blocks(bits, context);

    for (size_t i = 0; i < macroblock.size(); i++) {
        ASSERT_EQ(macroblock[i].r, 1) << "Index: " << i;
        ASSERT_EQ(macroblock[i].g, 1);
        ASSERT_EQ(macroblock[i].b, 1);
    }
}

TEST(MPEG1, read_predictive_macroblock_and_block_data) {
    auto data = make_bytes(0x77, 0x7f, 0x4f, 0x54);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    bits.read_bits_be(1);
    
    mpeg1::BlockContext context;
    context.picture = {
        .coding_type = mpeg1::CodingType::PredictiveCoded,
        .forward_f_code = 2
    };
    context.slice = {
        .quantizer_scale = 4
    };
    context.previous_macroblock_address = 18;
    context.macroblock = mpeg1::read_macroblock(bits, context.picture);

    EXPECT_EQ(context.macroblock.type.intra, false);
    EXPECT_EQ(context.macroblock.type.motion_backward, false);
    EXPECT_EQ(context.macroblock.type.motion_forward, true);
    EXPECT_EQ(context.macroblock.type.pattern, true);
    EXPECT_EQ(context.macroblock.type.quant, false);
    EXPECT_EQ(context.macroblock.motion_horizontal_forward_code, 0);
    EXPECT_EQ(context.macroblock.motion_vertical_forward_code, -1);
    EXPECT_EQ(context.macroblock.coded_block_pattern, 28);

    std::array<int, 64> block;

    block = mpeg1::read_block(bits, context, 1);
    EXPECT_EQ(block[0], -7);

    block = mpeg1::read_block(bits, context, 2);
    EXPECT_EQ(block[0], 7);
    EXPECT_EQ(block[8], -7);

    block = mpeg1::read_block(bits, context, 3);
    EXPECT_EQ(block[0], 7);
}
