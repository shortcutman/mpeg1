
//------------------------------------------------------------------------------
// copy.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/copy.hpp"

#include <gtest/gtest.h>

TEST(Copy, copy_block_mv_from_image_no_movement) {
    auto source = image::Frame::make_frame(16, 16);
    
    for (size_t i = 0; i < 256; i++) {
        source.image[i] = image::Colour{.r = 50, .g = 100, .b = 150};
    }

    auto copy = mpeg1::copy_block_mv_from_image(0, std::make_tuple(0, 0), source);

    for (size_t i = 0; i < 64; i++) {
        image::Colour c{.r = 50, .g = 100, .b = 150};
        EXPECT_EQ(copy[i], c);
    }
}

TEST(Copy, copy_block_mv_from_image_movement) {
    auto source = image::Frame::make_frame(32, 32);
    
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            source.image[y * 32 + x] = image::Colour{.r = 50, .g = 100, .b = 150};
        }
    }

    auto copy_no_mv = mpeg1::copy_block_mv_from_image(2, std::make_tuple(0, 0), source);
    for (size_t i = 0; i < 256; i++) {
        image::Colour c{};
        EXPECT_EQ(copy_no_mv[i], c);
    }

    auto copy_mv_down = mpeg1::copy_block_mv_from_image(2, std::make_tuple(0, -4), source);
    for (size_t i = 0; i < 32; i++) {
        image::Colour c{.r = 50, .g = 100, .b = 150};
        EXPECT_EQ(copy_mv_down[i], c) << "Position: " << i;
    }
    for (size_t i = 32; i < 256; i++) {
        image::Colour c{.r = 0, .g = 0, .b = 0};
        EXPECT_EQ(copy_mv_down[i], c) << "Position: " << i;
    }

    auto copy_mv_left = mpeg1::copy_block_mv_from_image(1, std::make_tuple(-4, 0), source);
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            if (x < 2) {
                image::Colour c{.r = 50, .g = 100, .b = 150};
                EXPECT_EQ(copy_mv_left[y * 16 + x], c) << "Position x: " << x << " y: " << y;
            } else {
                image::Colour c{.r = 0, .g = 0, .b = 0};
                EXPECT_EQ(copy_mv_left[y * 16 + x], c) << "Position x: " << x << " y: " << y;
            }
        }
    }
}

TEST(Copy, copy_block_mv_from_half_movement) {
    auto source = image::Frame::make_frame(32, 32);
    
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            source.image[y * 32 + x] = image::Colour{.y = 50};
        }
    }

    auto copy_mv_down = mpeg1::copy_block_mv_from_image(2, std::make_tuple(0, -1), source);
    for (size_t i = 0; i < 16; i++) {
        image::Colour c{.y = 25};
        EXPECT_EQ(copy_mv_down[i], c) << "Position: " << i;
    }
    for (size_t i = 16; i < 256; i++) {
        image::Colour c{.r = 0, .g = 0, .b = 0};
        EXPECT_EQ(copy_mv_down[i], c) << "Position: " << i;
    }

    auto copy_mv_left = mpeg1::copy_block_mv_from_image(1, std::make_tuple(-1, 0), source);
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            if (x < 1) {
                image::Colour c{.r = 25, .g = 0, .b = 0};
                EXPECT_EQ(copy_mv_left[y * 16 + x], c) << "Position x: " << x << " y: " << y;
            } else {
                image::Colour c{.r = 0, .g = 0, .b = 0};
                EXPECT_EQ(copy_mv_left[y * 16 + x], c) << "Position x: " << x << " y: " << y;
            }
        }
    }
}
