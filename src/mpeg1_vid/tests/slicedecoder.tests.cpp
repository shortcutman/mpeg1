
//------------------------------------------------------------------------------
// slicedecoder.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/slicedecoder.hpp"

#include "colour.hpp"
#include "util.hpp"

#include <algorithm>

#include <gtest/gtest.h>

class ToTestSliceDecoder : public mpeg1::SliceDecoder {
public:
    ToTestSliceDecoder(mpeg1::SequenceHeader sequence, mpeg1::PictureHeader picture) : mpeg1::SliceDecoder(sequence, picture) {}

    FRIEND_TEST(SliceDecoderTest, read_intra_macroblock_and_block_data);
    FRIEND_TEST(SliceDecoderTest, read_predictive_macroblock_and_block_data);
};

TEST(SliceDecoderTest, read_intra_macroblock_and_block_data) {
    auto data = util::make_bytes(
	0x00, 0x00, 0x01, 0x01, 0x23, 0xf8, 0x85, 0x29,
	0x48, 0x8b);

    auto span = std::span<std::byte>(data);

    mpeg1::SequenceHeader sequence{.horizontal_size = 16, .vertical_size = 16};
    mpeg1::PictureHeader pic_header{ .coding_type = mpeg1::CodingType::IntraCoded };
    ToTestSliceDecoder slice_dec(sequence, pic_header);

    auto frame = image::Frame::make_frame(16, 16);
    auto addr = slice_dec.decode(span, image::Frame{}, frame);
    EXPECT_EQ(addr, 0);

    ASSERT_TRUE(std::all_of(frame.image.begin(), frame.image.end(), std::bind(std::equal_to<>(), std::placeholders::_1, image::Colour{.y = 17, .cb = 128, .cr = 128})));
}

TEST(SliceDecoderTest, read_predictive_macroblock_and_block_data) {
    auto data = util::make_bytes(0x77, 0x7f, 0x4f, 0x54);
    auto span = std::span<std::byte>(data);
    auto bits = util::bitspan(span);
    bits.read_bits_be(1);

    mpeg1::SequenceHeader sequence{.horizontal_size = 16, .vertical_size = 16};
    auto picture = mpeg1::PictureHeader{
        .coding_type = mpeg1::CodingType::PredictiveCoded,
        .forward_f_code = 2
    };

    ToTestSliceDecoder slice_dec(sequence, picture);
    slice_dec.set_slice(mpeg1::SliceHeader{
        .quantizer_scale = 4
    });

    auto macroblock = mpeg1::read_macroblock(bits, picture);

    EXPECT_EQ(macroblock.type.intra, false);
    EXPECT_EQ(macroblock.type.motion_backward, false);
    EXPECT_EQ(macroblock.type.motion_forward, true);
    EXPECT_EQ(macroblock.type.pattern, true);
    EXPECT_EQ(macroblock.type.quant, false);
    EXPECT_EQ(macroblock.motion_horizontal_forward_code, 0);
    EXPECT_EQ(macroblock.motion_vertical_forward_code, -1);
    EXPECT_EQ(macroblock.coded_block_pattern, 28);

    std::array<int, 64> block;

    block = slice_dec.read_block(bits, 1);

    EXPECT_TRUE(std::all_of(block.begin(), block.end(), std::bind(std::equal_to<>(), std::placeholders::_1, -1)));

    block = slice_dec.read_block(bits, 2);
    EXPECT_TRUE(std::all_of(block.begin(), block.begin() + 32, std::bind(std::equal_to<>(), std::placeholders::_1, 0)));
    EXPECT_TRUE(std::all_of(block.begin() + 32, block.begin() + 40, std::bind(std::equal_to<>(), std::placeholders::_1, 1)));
    EXPECT_TRUE(std::all_of(block.begin() + 40, block.begin() + 56, std::bind(std::equal_to<>(), std::placeholders::_1, 2)));
    EXPECT_TRUE(std::all_of(block.begin() + 56, block.end(), std::bind(std::equal_to<>(), std::placeholders::_1, 3)));

    block = slice_dec.read_block(bits, 3);
    EXPECT_TRUE(std::all_of(block.begin(), block.end(), std::bind(std::equal_to<>(), std::placeholders::_1, 1)));
}
