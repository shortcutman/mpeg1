
//------------------------------------------------------------------------------
// decoder.integration.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/decoder.hpp"

#include "util.hpp"

#include <gtest/gtest.h>
#include <fstream>
#include <print>
#include <filesystem>

namespace {

std::vector<std::byte> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

}

TEST(DecoderIntegration, black_I_frame) {
    auto input = read_file("../src/mpeg1_vid/tests/data/black_I_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    std::stringstream ss;
    image::writeOutPPM(ss, frame_one->encoded_width, frame_one->encoded_height, frame_one->image);
    auto contents = ss.str();

    auto expected = read_file("../src/mpeg1_vid/tests/data/black_I_frame.ppm");

    ASSERT_TRUE(std::equal(contents.begin(), contents.end(), expected.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        }));
}

TEST(DecoderIntegration, black_I_frame_ycbcr) {
    auto input = read_file("../src/mpeg1_vid/tests/data/black_I_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d(mpeg1::Decoder::ImageFormat::YCbCr);
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    for (uint32_t i = 0; i < frame_one->encoded_width * frame_one->encoded_height; i++) {
        ASSERT_EQ(frame_one->image[i].y, frame_one->luma[i]) << "Luma Fail at pixel: " << i;
    }

    for (uint32_t y = 0; y < frame_one->encoded_height; y += 2) {
        for (uint32_t x = 0; x < frame_one->encoded_width; x += 2) {
            ASSERT_EQ(frame_one->image[y * frame_one->encoded_width + x].cb,
                      frame_one->cb[y / 2 * frame_one->cb.width() + x / 2])
                      << "Cb Fail at pixel y:" << y << " x: " << x;

            ASSERT_EQ(frame_one->image[y * frame_one->encoded_width + x].cr,
                      frame_one->cr[y / 2 * frame_one->cr.width() + x / 2])
                      << "Cr Fail at pixel y:" << y << " x: " << x;
        }
    }
}

TEST(DecoderIntegration, eye_I_frame_P_frame) {
    auto input = read_file("../src/mpeg1_vid/tests/data/eye_I_frame_P_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    std::stringstream ss;

    image::writeOutPPM(ss, frame_one->encoded_width, frame_one->encoded_height, frame_one->image);
    auto contents = ss.str();

    auto expected = read_file("../src/mpeg1_vid/tests/data/eye_I_frame.ppm");

    ASSERT_TRUE(std::equal(contents.begin(), contents.end(), expected.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        }));

    ss.str("");

    auto frame_two = d.next_frame();
    ASSERT_EQ(frame_two->encoded_width, 640);
    ASSERT_EQ(frame_two->encoded_height, 272);

    image::writeOutPPM(ss, frame_two->encoded_width, frame_two->encoded_height, frame_two->image);
    auto contents2 = ss.str();
    auto expected2 = read_file("../src/mpeg1_vid/tests/data/eye_P_frame.ppm");

    ASSERT_TRUE(std::equal(contents2.begin(), contents2.end(), expected2.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        }));
}

TEST(DecoderIntegration, eye_I_frame_P_frame_ycbcr) {
    auto input = read_file("../src/mpeg1_vid/tests/data/eye_I_frame_P_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d(mpeg1::Decoder::ImageFormat::YCbCr);
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    for (uint32_t i = 0; i < frame_one->encoded_width * frame_one->encoded_height; i++) {
        ASSERT_EQ(frame_one->image[i].y, frame_one->luma[i]) << "Fail at pixel: " << i;
    }

    for (uint32_t y = 0; y < frame_one->encoded_height; y += 2) {
        for (uint32_t x = 0; x < frame_one->encoded_width; x += 2) {
            ASSERT_EQ(frame_one->image[y * frame_one->encoded_width + x].cb,
                      frame_one->cb[(y / 2) * frame_one->cb.width() + x / 2])
                      << "Cb Fail at pixel y:" << y << " x: " << x;

            ASSERT_EQ(frame_one->image[y * frame_one->encoded_width + x].cr,
                      frame_one->cr[y / 2 * frame_one->cr.width() + x / 2])
                      << "Cr Fail at pixel y:" << y << " x: " << x;
        }
    }

    auto frame_two = d.next_frame();
    ASSERT_EQ(frame_two->encoded_width, 640);
    ASSERT_EQ(frame_two->encoded_height, 272);

    for (uint32_t i = 0; i < frame_two->encoded_width * frame_two->encoded_height; i++) {
        ASSERT_EQ(frame_two->image[i].y, frame_two->luma[i]) << "Fail at pixel: " << i;
    }

    for (uint32_t y = 0; y < frame_two->encoded_height; y += 2) {
        for (uint32_t x = 0; x < frame_two->encoded_width; x += 2) {
            ASSERT_EQ(frame_two->image[y * frame_two->encoded_width + x].cb,
                      frame_two->cb[(y / 2) * frame_two->cb.width() + x / 2])
                      << "Cb Fail at pixel y:" << y << " x: " << x;

            ASSERT_EQ(frame_two->image[y * frame_two->encoded_width + x].cr,
                      frame_two->cr[y / 2 * frame_two->cr.width() + x / 2])
                      << "Cr Fail at pixel y:" << y << " x: " << x;
        }
    }
}

TEST(DecoderIntegration, eye_skip_I_frame_P_frame) {
    auto input = read_file("../src/mpeg1_vid/tests/data/eye_I_frame_P_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame(false);
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();
    ASSERT_EQ(frame_one->encoded_width, 640);
    ASSERT_EQ(frame_one->encoded_height, 272);

    for (auto c : frame_one->image) {
        const auto check = image::Colour{.r = 0, .g = 135, .b =0};
        ASSERT_EQ(c, check);
    }

    std::stringstream ss;

    auto frame_two = d.next_frame();
    ASSERT_EQ(frame_two->encoded_width, 640);
    ASSERT_EQ(frame_two->encoded_height, 272);

    uint64_t count = 0;
    for (auto c : frame_two->image) {
        const auto check = image::Colour{.r = 0, .g = 135, .b =0};
        if (c == check) {
            count++;
        }
    }

    ASSERT_EQ(count, 108252);
}

TEST(DecoderIntegration, frame_end_macroblock_skip_next_pic_header) {
    auto input = read_file("../src/mpeg1_vid/tests/data/frame_end_macroblock_skip.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    for (size_t i = 0; i < 8; i++) {
        auto frame = d.next_frame(); 
        EXPECT_TRUE(frame.has_value()) << frame.error().what();
    }
}

TEST(DecoderIntegration, frame_end_macroblock_skip_eof) {
    auto input = read_file("../src/mpeg1_vid/tests/data/frame_end_macroblock_skip.es");
    ASSERT_FALSE(input.empty());

    auto adjusted_span = std::span<std::byte>(input.begin(), input.end() - 9);

    mpeg1::Decoder d;
    d.set_data(adjusted_span);

    for (size_t i = 0; i < 8; i++) {
        auto frame = d.next_frame(); 
        EXPECT_TRUE(frame.has_value()) << frame.error().what();
    }
}
