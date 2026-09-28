
//------------------------------------------------------------------------------
// vlc.tests.cpp
//------------------------------------------------------------------------------

#include "vlc.hpp"
#include "mpeg1_vid/vlctables.hpp"

#include <gtest/gtest.h>

#include <array>

namespace {

// https://stackoverflow.com/a/45172360
template<typename... Ts>
std::array<std::byte, sizeof...(Ts)> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

}

TEST(VLC, optimisation_7bitcode_16bitadvance_extendedzeroes) {
    auto data = make_bytes(0x0c, 0x00, 0x00, 0x00);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs);
    EXPECT_EQ(bs.bits_read(), 7);
    EXPECT_EQ(result, 9);
}

TEST(VLC, optimisation_7bitcode_16bitadvance_extendedones) {
    auto data = make_bytes(0x0d, 0xff, 0xff, 0xff);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs);
    EXPECT_EQ(bs.bits_read(), 7);
    EXPECT_EQ(result, 9);
}

TEST(VLC, optimisation_11bitcode_16bitadvance_extendedzeroes) {
    auto data = make_bytes(0x03, 0x00, 0x00, 0x00);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs);
    EXPECT_EQ(bs.bits_read(), 11);
    EXPECT_EQ(result, 33);
}

TEST(VLC, optimisation_11bitcode_16bitadvance_extendedones) {
    auto data = make_bytes(0x03, 0x3f, 0xff, 0xff);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs);
    EXPECT_EQ(bs.bits_read(), 11);
    EXPECT_EQ(result, 32);
}

TEST(VLC, optimisation_11bitcode2_16bitadvance_extendedones) {
    auto data = make_bytes(0x03, 0x1f, 0xff, 0xff);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs);
    EXPECT_EQ(bs.bits_read(), 11);
    EXPECT_EQ(result, 33);
}

TEST(VLC, straight_11bitcode_16bitadvance_extendedzeroes) {
    auto data = make_bytes(0x03, 0x00, 0x00, 0x00);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol(bs);
    EXPECT_EQ(bs.bits_read(), 11);
    EXPECT_EQ(result, 33);
}

TEST(VLC, straight_11bitcode_16bitadvance_extendedones) {
    auto data = make_bytes(0x03, 0x1f, 0xff, 0xff);
    util::bitspan bs(data);
    auto result = mpeg1::MACROBLOCK_ADDRESSING.next_symbol(bs);
    EXPECT_EQ(bs.bits_read(), 11);
    EXPECT_EQ(result, 33);
}

TEST(VLC, optimisation_nocode) {
    auto data = make_bytes(0x00, 0x00, 0x00, 0x00);
    util::bitspan bs(data);
    EXPECT_THROW(mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs), std::runtime_error);
    EXPECT_EQ(bs.bits_read(), 0);
}

TEST(VLC, optimisation_nobytes) {
    auto data = make_bytes(0x00);
    util::bitspan bs(data);
    EXPECT_THROW(mpeg1::MACROBLOCK_ADDRESSING.next_symbol_opt(bs), std::runtime_error);
}

TEST(VLC, optimisation_max16bit) {
    auto data = make_bytes(0xff, 0xff);
    util::bitspan bs(data);
    EXPECT_EQ(mpeg1::MACROBLOCK_MOTION_VECTOR_CODES.next_symbol_opt(bs), 0);
}
