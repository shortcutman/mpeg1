
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
