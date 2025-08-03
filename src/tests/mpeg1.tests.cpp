
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
    auto data = make_bytes(0x00, 0x00, 0x01, 0x00, 0x00, 0x0f, 0xff);
    auto span = std::span<std::byte>(data);
    auto header = mpeg1::read_picture_header(span);

    EXPECT_EQ(header.temporal_reference, 0);
    EXPECT_EQ(header.coding_type, mpeg1::CodingType::IntraCoded);
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
    auto block = mpeg1::read_macroblock(bits, mpeg1::CodingType::IntraCoded);

    EXPECT_EQ(block.address_increment, 1);
    EXPECT_EQ(block.quant, false);
    EXPECT_EQ(block.motion_forward, false);
    EXPECT_EQ(block.motion_backward, false);
    EXPECT_EQ(block.pattern, false);
    EXPECT_EQ(block.intra, true);
}

TEST(MPEG1, read_block_unhandled) {
    auto data = make_bytes(0x00, 0x00, 0x01, 0x01, 0x13, 0xf8, 0x00, 0x00);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    mpeg1::read_slice_header(bits); //align test data
    EXPECT_THROW(mpeg1::read_macroblock(bits, mpeg1::CodingType::PredictiveCoded), std::runtime_error);
    EXPECT_THROW(mpeg1::read_macroblock(bits, mpeg1::CodingType::BidirectionalPredCoded), std::runtime_error);
}