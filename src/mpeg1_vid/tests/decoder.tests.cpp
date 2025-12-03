//------------------------------------------------------------------------------
// decoder.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/decoder.hpp"

#include "util.hpp"

#include <gtest/gtest.h>

class ToTestDecoder : public mpeg1::Decoder {
public:
    FRIEND_TEST(DecoderTest, next_code_no_value);
    FRIEND_TEST(DecoderTest, next_code_picture);
};

TEST(DecoderTest, next_code_no_value) {
    ToTestDecoder d;

    EXPECT_EQ(d.next_code().has_value(), false);
    EXPECT_THROW(throw d.next_code().error(), std::runtime_error);
}

TEST(DecoderTest, next_code_picture) {
    ToTestDecoder d;

    auto data = util::make_bytes(0x00, 0x00, 0x01, 0xb8);
    d.set_data(data);
    auto next_code = d.next_code();
    ASSERT_EQ(next_code.has_value(), true);

    auto [code, code_data] = *next_code;
    EXPECT_EQ(code, 0x000001b8);
    EXPECT_EQ(&code_data[0], &data[0]);
}

TEST(DecoderTest, next_frame_no_bytes) {
    ToTestDecoder d;

    EXPECT_EQ(d.next_frame().has_value(), false);
    EXPECT_THROW(throw d.next_frame().error(), std::runtime_error);
}
