
//------------------------------------------------------------------------------
// audiodecoder.hpp
//------------------------------------------------------------------------------

#pragma once

#include "bitspan.hpp"

#include <expected>
#include <span>

namespace mpeg1_aud {
    
    struct FrameHeader {
        uint32_t syncword;
        uint32_t id;
        uint32_t layer;
        uint32_t protection_bit;
        uint32_t bitrate_index;
        uint32_t sampling_frequency;
        uint32_t padding_bit;
        uint32_t private_bit;
        uint32_t mode;
        uint32_t mode_ext;
        uint32_t copyright;
        uint32_t original;
        uint32_t emphasis;
    };

    void align_to_sync(std::span<std::byte>& data);
    FrameHeader read_frame_header(std::span<std::byte>& data);
    
    typedef std::array<std::array<int32_t, 32>, 2> ChannelValues;

    ChannelValues read_allocations(util::bitspan& data, uint32_t bound, uint32_t sblimit);
    ChannelValues read_scfsi(util::bitspan& data, ChannelValues& allocations, uint32_t bound, uint32_t sblimit);

    typedef std::array<std::array<std::array<int32_t, 3>, 32>, 2> ScaleFactors;
    ScaleFactors read_scale_factors(util::bitspan& data, ChannelValues& allocations, ChannelValues& scfsi, uint32_t sblimit, uint32_t channels);

    std::array<int32_t, 3> read_samples(util::bitspan& data, int32_t level, int32_t scale_factor);

    typedef std::array<short, 1152 * 2> DecodedSamples;

    DecodedSamples decode_samples(util::bitspan& data, ChannelValues& allocations, ScaleFactors& sf, uint32_t bound, uint32_t sblimit);

    class Decoder {
    public:
        typedef std::span<std::byte> Data;

    private:
        Data _data;

    public:
        Decoder() {}
        ~Decoder() {}

        void set_data(Data data);
        size_t next_frame(DecodedSamples& samples);
    };
}
