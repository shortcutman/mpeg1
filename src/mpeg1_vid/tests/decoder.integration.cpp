
//------------------------------------------------------------------------------
// decoder.integration.cpp
//------------------------------------------------------------------------------

#include "mpeg1_vid/decoder.hpp"
#include "mpeg1_vid/decode.hpp"

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

TEST(DecoderIntegration, test_one) {
    std::println("cwd: {}", std::filesystem::current_path().string());

    auto input = read_file("../src/mpeg1_vid/tests/data/black_I_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();

    std::stringstream ss;
    image::writeOutPPM(ss, 640, 272, frame_one.value());
    image::writeOutPPM("/tmp/danpg1/test.ppm", 640, 272, frame_one.value());
    auto contents = ss.str();

    auto expected = read_file("../src/mpeg1_vid/tests/data/black_I_frame.ppm");

    ASSERT_TRUE(std::equal(contents.begin(), contents.end(), expected.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        }));
}

TEST(DecoderIntegration, test_two) {
    std::println("cwd: {}", std::filesystem::current_path().string());

    auto input = read_file("../src/mpeg1_vid/tests/data/eye_I_frame_P_frame.es");
    ASSERT_FALSE(input.empty());

    mpeg1::Decoder d;
    d.set_data(input);

    auto frame_one = d.next_frame();
    ASSERT_TRUE(frame_one.has_value()) << frame_one.error().what();

    std::stringstream ss;

    image::writeOutPPM(ss, 640, 272, frame_one.value());
    auto contents = ss.str();

    auto expected = read_file("../src/mpeg1_vid/tests/data/eye_I_frame.ppm");

    ASSERT_TRUE(std::equal(contents.begin(), contents.end(), expected.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        }));

    ss.str("");

    auto frame_two = d.next_frame();

    image::writeOutPPM(ss, 640, 272, frame_two.value());
    auto contents2 = ss.str();
    auto expected2 = read_file("../src/mpeg1_vid/tests/data/eye_P_frame.ppm");

    ASSERT_TRUE(std::equal(contents2.begin(), contents2.end(), expected2.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        }));
}
