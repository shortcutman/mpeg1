
//------------------------------------------------------------------------------
// comparison.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1_aud/audiodecoder.hpp"

#include <fstream>

#include <gtest/gtest.h>

extern "C" {
#include "kjmp2.h"
}

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

TEST(AudioDecoderIntegration, compare_to_kjmp2) {
    kjmp2_context_t kctx;
    kjmp2_init(&kctx);

    auto audio = read_file("../src/mpeg1_aud/tests/data/audio.mp2");
    auto span = std::span(audio);
    mpeg1_aud::align_to_sync(span);
    // auto total = 0;

    mpeg1_aud::Decoder decoder;
    decoder.set_data(span);
    mpeg1_aud::DecodedSamples samples;

    for (size_t f = 0; f < 5000; f++) {
        std::array<signed short, 1152*2> frame{};

        mpeg1_aud::align_to_sync(span);
        // std::println("Frame: {}, File offset: {}", f, (audio.size() - span.size()));

        auto bytes_read = kjmp2_decode_frame(&kctx, reinterpret_cast<unsigned char*>(span.data()), frame.data());
        // total += bytes_read;
        // std::println("Bytes read: {}, total: {}", bytes_read, total);

        auto frame_size = decoder.next_frame(samples);

        ASSERT_EQ(bytes_read, frame_size) << "Frame: " << f;
        for (size_t i = 0; i < (1152 * 2); i++) {
            ASSERT_EQ(frame[i], samples[i]) << "Pos: " << i;
        }

        span = span.subspan(bytes_read);
    }
}
