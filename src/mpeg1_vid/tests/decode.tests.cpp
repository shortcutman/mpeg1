
//------------------------------------------------------------------------------
// decode.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/decode.hpp"

#include "mpeg1.consts.hpp"

#include <gtest/gtest.h>

namespace {

template<typename... Ts>
std::vector<std::byte> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

}

namespace {
    void test_code(std::vector<std::byte> data, uint32_t expected) {
        auto result = mpeg1::get_code(std::span<std::byte>(data));
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(*result, expected);    
    }
}

TEST(MPEG1_Decode, get_code_sequence) {
    test_code(make_bytes(0x00, 0x00, 0x01, 0xb3), mpeg1::start_code::sequence);
}

TEST(MPEG1_Decode, get_code_gop) {
    test_code(make_bytes(0x00, 0x00, 0x01, 0xb8), mpeg1::start_code::group_of_pictures);
}

TEST(MPEG1_Decode, get_code_picture) {
    test_code(make_bytes(0x00, 0x00, 0x01, 0xb8), mpeg1::start_code::group_of_pictures);
}

TEST(MPEG1_Decode, get_code_slice) {
    test_code(make_bytes(0x00, 0x00, 0x01, 0x01), mpeg1::start_code::slice_minimum);
    test_code(make_bytes(0x00, 0x00, 0x01, 0x11), uint32_t{0x0111});
    test_code(make_bytes(0x00, 0x00, 0x01, 0xaf), mpeg1::start_code::slice_maximum);
}
