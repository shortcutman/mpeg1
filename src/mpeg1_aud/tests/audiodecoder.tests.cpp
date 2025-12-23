
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
}
