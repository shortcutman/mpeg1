
//------------------------------------------------------------------------------
// copy.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/copy.hpp"

#include "util.hpp"

#include <algorithm>
#include <gtest/gtest.h>

TEST(Copy, copy_block_to_buff) {
    image::PixelBuffer buff(8, 8);
    std::fill(buff.begin(), buff.end(), 0);

    mpeg1::Block block;
    std::fill(block.begin(), block.end(), 1);

    mpeg1::add_block_to_buff(0, block, buff);
    for (auto i : buff) {
        EXPECT_EQ(i, 1);
    }
}

TEST(Copy, add_block_to_buff) {
    image::PixelBuffer buff(8, 8);
    std::fill(buff.begin(), buff.end(), 1);

    mpeg1::Block block;
    std::fill(block.begin(), block.end(), 2);

    mpeg1::add_block_to_buff(0, block, buff);
    for (auto i : buff) {
        EXPECT_EQ(i, 3);
    }
}

TEST(Copy, copy_block_mv_from_image_no_movement) {
    auto source = image::Frame::make_frame(16, 16);
    auto destination = image::Frame::make_frame(16, 16);

    std::fill(source.luma.begin(), source.luma.end(), 50);
    std::fill(source.cb.begin(), source.cb.end(), 100);
    std::fill(source.cr.begin(), source.cr.end(), 150);

    mpeg1::copy_mv_block(0, std::make_tuple(0, 0), source, destination);

    ASSERT_TRUE(std::all_of(destination.luma.begin(), destination.luma.end(), [](auto i) { return i == 50;}));
    ASSERT_TRUE(std::all_of(destination.cb.begin(), destination.cb.end(), [](auto i) { return i == 100;}));
    ASSERT_TRUE(std::all_of(destination.cr.begin(), destination.cr.end(), [](auto i) { return i == 150;}));
}

TEST(Copy, copy_block_mv_from_image_movement) {
    auto source = image::Frame::make_frame(32, 32);
    
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            source.luma.at(y * 32 + x) = 50;
            source.cb.at((y / 2) * 16 + x/2) = 100;
            source.cr.at((y / 2) * 16 + x/2) = 150;
        }
    }
    // image::writeOutPPM("/tmp/danpg1/source.ppm", source);
    
    auto copy_mv_down = image::Frame::make_frame(32, 32);
    mpeg1::copy_mv_block(2, std::make_tuple(0, -4), source, copy_mv_down);
    // image::writeOutPPM("/tmp/danpg1/copy_mv_down.ppm", copy_mv_down);
    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/copy_block_mv_from_image_movement_mv_down.ppm", copy_mv_down));

    auto copy_mv_left = image::Frame::make_frame(32, 32);
    mpeg1::copy_mv_block(1, std::make_tuple(-4, 0), source, copy_mv_left);
    // image::writeOutPPM("/tmp/danpg1/copy_mv_left.ppm", copy_mv_left);
    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/copy_block_mv_from_image_movement_mv_left.ppm", copy_mv_left));
}

TEST(Copy, copy_block_mv_from_half_movement) {
    auto source = image::Frame::make_frame(32, 32);
    
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            source.luma.at(y * 32 + x) = 50;
            source.cb.at((y / 2) * 16 + x/2) = 100;
            source.cr.at((y / 2) * 16 + x/2) = 150;
        }
    }
    // image::writeOutPPM("/tmp/danpg1/source.ppm", source);

    auto copy_mv_down = image::Frame::make_frame(32, 32);
    mpeg1::copy_mv_block(2, std::make_tuple(0, -1), source, copy_mv_down);
    // image::writeOutPPM("/tmp/danpg1/copy_mv_down.ppm", copy_mv_down);
    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/copy_block_mv_from_half_movement_mv_down.ppm", copy_mv_down));

    auto copy_mv_left = image::Frame::make_frame(32, 32);
    mpeg1::copy_mv_block(1, std::make_tuple(-1, 0), source, copy_mv_left);
    // image::writeOutPPM("/tmp/danpg1/copy_mv_left.ppm", copy_mv_left);
    ASSERT_TRUE(util::compare_image("../src/mpeg1_vid/tests/data/copy_block_mv_from_half_movement_mv_left.ppm", copy_mv_left));
}
