
//------------------------------------------------------------------------------
// decoder.integration.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/decoder.hpp"

#include "util.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <fstream>
#include <print>
#include <filesystem>

TEST(DecoderIntegration, black_I_frame) {
    auto input = util::read_file("../src/mpeg1_vid/tests/data/black_I_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);
    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/black_I_frame.ppm", *frame_one));
}

TEST(DecoderIntegration, eye_I_frame_P_frame) {
    auto input = util::read_file("../src/mpeg1_vid/tests/data/eye_I_frame_P_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/eye_I_frame.ppm", *frame_one));

    auto frame_two = d.next_frame();
    ASSERT_EQ(frame_two->encoded_width, 640);
    ASSERT_EQ(frame_two->encoded_height, 272);

    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/eye_P_frame.ppm", *frame_two));
}

TEST(DecoderIntegration, eye_skip_I_frame_P_frame) {
    auto input = util::read_file("../src/mpeg1_vid/tests/data/eye_I_frame_P_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame(false);
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    ASSERT_TRUE(std::all_of(frame_one->luma.begin(), frame_one->luma.end(), [](int32_t i) { return i == 0;}));
    ASSERT_TRUE(std::all_of(frame_one->cb.begin(), frame_one->cb.end(), [](int32_t i) { return i == 0;}));
    ASSERT_TRUE(std::all_of(frame_one->cr.begin(), frame_one->cr.end(), [](int32_t i) { return i == 0;}));

    auto frame_two = d.next_frame();
    ASSERT_EQ(frame_two->encoded_width, 640);
    ASSERT_EQ(frame_two->encoded_height, 272);
    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/post_skip_frame.ppm", *frame_two));
}

TEST(DecoderIntegration, frame_end_macroblock_skip_next_pic_header) {
    auto input = util::read_file("../src/mpeg1_vid/tests/data/frame_end_macroblock_skip.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    for (size_t i = 0; i < 8; i++) {
        auto frame = d.next_frame(); 
        EXPECT_TRUE(frame.has_value()) << frame.error().what();
    }
}

TEST(DecoderIntegration, frame_end_macroblock_skip_eof) {
    auto input = util::read_file("../src/mpeg1_vid/tests/data/frame_end_macroblock_skip.es");
    ASSERT_FALSE(input.empty());

    auto adjusted_span = std::span<std::byte>(input.begin(), input.end() - 9);

    mpeg1::Decoder d;
    d.set_data(adjusted_span);

    for (size_t i = 0; i < 8; i++) {
        auto frame = d.next_frame(); 
        EXPECT_TRUE(frame.has_value()) << frame.error().what();
    }
}
