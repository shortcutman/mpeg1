
//------------------------------------------------------------------------------
// bitspan.tests.cpp
//------------------------------------------------------------------------------

#include "bitspan.hpp"

#include <gtest/gtest.h>

namespace {

// https://stackoverflow.com/a/45172360
template<typename... Ts>
std::array<std::byte, sizeof...(Ts)> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

}

using namespace util;

TEST(BitSpan, peak_and_read_0bits) {
    auto data = make_bytes(0xff);
    auto span = bitspan(std::span{data});

    EXPECT_EQ(span.peek_bits_le(0), 0x00);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_EQ(span.read_bits_le(0), 0x00);
    EXPECT_EQ(span.bits_read(), 0);
}

TEST(BitSpan, peak_and_read_4bits_from_8bits) {
    auto data = make_bytes(0xff);
    auto span = bitspan(std::span{data});

    EXPECT_EQ(span.peek_bits_le(4), 0x0f);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_EQ(span.read_bits_le(4), 0x0f);
    EXPECT_EQ(span.bits_read(), 4);
}

TEST(BitSpan, peak_and_read_8bits_from_8bits) {
    auto data = make_bytes(0xff);
    auto span = bitspan(std::span{data});

    EXPECT_EQ(span.peek_bits_le(8), 0xff);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_EQ(span.read_bits_le(8), 0xff);
    EXPECT_EQ(span.bits_read(), 8);
}

TEST(BitSpan, fail_to_peek_and_read_16bits_from_8bits) {
    auto data = make_bytes(0xff);
    auto span = bitspan(std::span{data});

    EXPECT_THROW(span.peek_bits_le(16), std::runtime_error);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_THROW(span.read_bits_le(16), std::runtime_error);
    EXPECT_EQ(span.bits_read(), 0);
}

TEST(BitSpan, peak_and_read_12bits_from_16bits) {
    auto data = make_bytes(0xff, 0x88);
    auto span = bitspan(std::span{data});

    EXPECT_EQ(span.peek_bits_le(12), 0x8ff);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_EQ(span.read_bits_le(12), 0x8ff);
    EXPECT_EQ(span.bits_read(), 12);
}

TEST(BitSpan, peak_and_read_16bits_from_16bits) {
    auto data = make_bytes(0xff, 0x88);
    auto span = bitspan(std::span{data});

    EXPECT_EQ(span.peek_bits_le(16), 0x88ff);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_EQ(span.read_bits_le(16), 0x88ff);
    EXPECT_EQ(span.bits_read(), 16);
}

TEST(BitSpan, fail_to_peak_and_read_20bits_from_16bits) {
    auto data = make_bytes(0xff, 0x88);
    auto span = bitspan(std::span{data});

    EXPECT_THROW(span.peek_bits_le(20), std::runtime_error);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_THROW(span.read_bits_le(20), std::runtime_error);
    EXPECT_EQ(span.bits_read(), 0);
}

TEST(BitSpan, subsequent_peeks_and_reads) {
    auto data = make_bytes(0xf8, 0xf8);
    auto span = bitspan(std::span{data});

    EXPECT_EQ(span.peek_bits_le(4), 0x8);
    EXPECT_EQ(span.bits_read(), 0);
    EXPECT_EQ(span.read_bits_le(4), 0x8);
    EXPECT_EQ(span.bits_read(), 4);
    EXPECT_EQ(span.peek_bits_le(4), 0xf);
    EXPECT_EQ(span.bits_read(), 4);
    EXPECT_EQ(span.read_bits_le(4), 0xf);
    EXPECT_EQ(span.bits_read(), 8);

    EXPECT_EQ(span.peek_bits_le(4), 0x8);
    EXPECT_EQ(span.bits_read(), 8);
    EXPECT_EQ(span.read_bits_le(4), 0x8);
    EXPECT_EQ(span.bits_read(), 12);
    EXPECT_EQ(span.peek_bits_le(4), 0xf);
    EXPECT_EQ(span.bits_read(), 12);
    EXPECT_EQ(span.read_bits_le(4), 0xf);
    EXPECT_EQ(span.bits_read(), 16);
}

TEST(BitSpan, round_to_next_byte_subbyte_reads) {
    auto data = make_bytes(0x12, 0x34);
    bitspan bits(data);

    EXPECT_EQ(bits.peek_bits_le(4), 0x02);
    EXPECT_EQ(bits.bits_read(), 0);
    bits.round_to_next_byte();
    EXPECT_EQ(bits.bits_read(), 0);

    EXPECT_EQ(bits.read_bits_le(4), 0x02);
    EXPECT_EQ(bits.bits_read(), 4);
    bits.round_to_next_byte();
    EXPECT_EQ(bits.bits_read(), 8);

    EXPECT_EQ(bits.read_bits_le(4), 0x04);
    EXPECT_EQ(bits.bits_read(), 12);
    bits.round_to_next_byte();
    EXPECT_EQ(bits.bits_read(), 16);
}

TEST(BitSpan, round_to_next_byte_fullbyte_reads) {
    auto data = make_bytes(0x12, 0x34);
    bitspan bits(data);

    EXPECT_EQ(bits.peek_bits_le(8), 0x12);
    EXPECT_EQ(bits.bits_read(), 0);
    bits.round_to_next_byte();
    EXPECT_EQ(bits.bits_read(), 0);

    EXPECT_EQ(bits.read_bits_le(8), 0x12);
    EXPECT_EQ(bits.bits_read(), 8);
    bits.round_to_next_byte();
    EXPECT_EQ(bits.bits_read(), 8);

    EXPECT_EQ(bits.read_bits_le(8), 0x34);
    EXPECT_EQ(bits.bits_read(), 16);
    bits.round_to_next_byte();
    EXPECT_EQ(bits.bits_read(), 16);
}
