
//------------------------------------------------------------------------------
// audiodecoder.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1_aud/audiodecoder.hpp"

#include <fstream>

#include <gtest/gtest.h>

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

TEST(AudioDecoder, align_to_syncword) {
    auto audio = read_file("../src/mpeg1_aud/tests/data/audio.mp2");
    auto span = std::span(audio);
    mpeg1_aud::align_to_sync(span);

    EXPECT_EQ(span[0], (std::byte{0xff}));
    EXPECT_EQ((span[1] & std::byte{0xf0}), (std::byte{0xf0}));
}

TEST(AudioDecoder, read_frame_header) {
    auto audio = read_file("../src/mpeg1_aud/tests/data/audio.mp2");
    auto span = std::span(audio);
    mpeg1_aud::align_to_sync(span);

    auto size_before_header = span.size();

    EXPECT_EQ(span[0], (std::byte{0xff}));
    EXPECT_EQ((span[1] & std::byte{0xf0}), (std::byte{0xf0}));

    auto header = mpeg1_aud::read_frame_header(span);
    EXPECT_EQ(header.syncword, 0xfff);
    EXPECT_EQ(header.layer, 2); //layer 2
    EXPECT_EQ(header.bitrate_index, 8); //128 kbits
    EXPECT_EQ(header.sampling_frequency, 0); //44.1kHz
    EXPECT_EQ(header.mode, 0); //stereo
    EXPECT_EQ(header.mode_ext, 0); //subbands 4-31 in intensity_stereo, bound==4

    EXPECT_EQ(size_before_header - span.size(), 4);

    mpeg1_aud::read_audio_data(span, header);
}

TEST(AudioDecoder, read_allocations) {
    auto audio = read_file("../src/mpeg1_aud/tests/data/audio.mp2");
    auto span = std::span(audio);
    mpeg1_aud::align_to_sync(span);
    span = span.subspan(4); //skip frame header

    mpeg1_aud::FrameHeader header{
        .mode = 0
    };

    util::bitspan bits(span);
    auto allocations = mpeg1_aud::read_allocations(bits, header);

    mpeg1_aud::Allocations expected{{
        {{31, 15, 15, 7, 3, 9, 5, 3, 3, 3, 5, 5, 3, 5, 7, 3,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {{31, 15, 15, 5, 3, 7, 5, 3, 3, 3, 3, 3, 3, 3, 5, 3,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    }};

    EXPECT_TRUE(std::equal(expected[0].begin(), expected[0].end(), allocations[0].begin()));
    EXPECT_TRUE(std::equal(expected[1].begin(), expected[1].end(), allocations[1].begin()));
    EXPECT_EQ(bits.bits_read(), 176);
}
