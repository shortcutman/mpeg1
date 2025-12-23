
//------------------------------------------------------------------------------
// audiodecoder.cpp
//------------------------------------------------------------------------------

#include "audiodecoder.hpp"

#include "bitspan.hpp"

#include <algorithm>
#include <vector>

void mpeg1_aud::align_to_sync(std::span<std::byte>& data) {
    util::bitspan bits(data);

    while (bits.peek_bits_be(12) != 0xfff) {
        bits.read_bits_be(4);
    }

    data = data.subspan(bits.bytes_read());
}

mpeg1_aud::FrameHeader mpeg1_aud::read_frame_header(std::span<std::byte>& data) {
    FrameHeader header;
    util::bitspan bits(data);

    header.syncword = bits.read_bits_be(12);
    header.id = bits.read_bits_be(1);
    header.layer = bits.read_bits_be(2);
    header.protection_bit = bits.read_bits_be(1);
    header.bitrate_index = bits.read_bits_be(4);
    header.sampling_frequency = bits.read_bits_be(2);
    header.padding_bit = bits.read_bits_be(1);
    header.private_bit = bits.read_bits_be(1);
    header.mode = bits.read_bits_be(2);
    header.mode_ext = bits.read_bits_be(2);
    header.copyright = bits.read_bits_be(1);
    header.original = bits.read_bits_be(1);
    header.emphasis = bits.read_bits_be(2);

    data = data.subspan(bits.bytes_read());

    return header;
}

namespace {
    std::array<uint32_t, 32> Bits_For_Subband_B2a = {
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2,
        0, 0, 0, 0, 0
    };
    std::array<std::vector<int>, 32> Level_For_Index_Subband = {{
        {{0, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65525}},
        {{0, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65525}},
        {{0, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  63, 127,  255,  511, 1023, 2047,  4095,  8191, 65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  7,  9, 15,  31,  65525}},
        {{0, 3, 5,  65525}},
        {{0, 3, 5,  65525}},
        {{0, 3, 5,  65525}},
        {{0, 3, 5,  65525}},
        {{}},
        {{}},
        {{}},
        {{}},
        {{}}
    }};
    const uint32_t SBLIMIT = 27; //table b.2a
}

void mpeg1_aud::read_audio_data(std::span<std::byte>& data, FrameHeader& header) {
    if (header.mode != 0) {
        return; //only stereo right now
    }

    util::bitspan bits(data);
    auto allocations = read_allocations(bits, header);
    (void)allocations;
}

mpeg1_aud::ChannelValues mpeg1_aud::read_allocations(util::bitspan& data, FrameHeader& header) {
    if (header.mode != 0) {
        throw std::runtime_error("Only stereo supported.");
    }

    ChannelValues allocation;
    std::fill(allocation[0].begin() + SBLIMIT, allocation[0].end(), 0);
    std::fill(allocation[1].begin() + SBLIMIT, allocation[1].end(), 0);
    for (size_t sb = 0; sb < SBLIMIT; sb++) {
        auto nbal = Bits_For_Subband_B2a[sb];
        allocation[0][sb] = Level_For_Index_Subband[sb][data.read_bits_be(nbal)];
        allocation[1][sb] = Level_For_Index_Subband[sb][data.read_bits_be(nbal)];
    }

    return allocation;
}

mpeg1_aud::ChannelValues mpeg1_aud::read_scfsi(util::bitspan& data, ChannelValues& allocations) {
    ChannelValues scfsi;
    std::fill(scfsi[0].begin(), scfsi[0].end(), 0);
    std::fill(scfsi[1].begin(), scfsi[1].end(), 0);

    for (size_t sb = 0; sb < SBLIMIT; sb++) {
        for (size_t ch = 0; ch < 2; ch++) {
            if (allocations[ch][sb]) {
                scfsi[ch][sb] = data.read_bits_be(2);
            }
        }
    }

    return scfsi;
}
