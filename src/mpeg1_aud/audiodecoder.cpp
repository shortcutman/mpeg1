
//------------------------------------------------------------------------------
// audiodecoder.cpp
//------------------------------------------------------------------------------

#include "audiodecoder.hpp"

#include "bitspan.hpp"

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
