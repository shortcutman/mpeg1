
//------------------------------------------------------------------------------
// audiodecoder.cpp
//------------------------------------------------------------------------------

#include "audiodecoder.hpp"

#include "bitspan.hpp"

#include <algorithm>
#include <cmath>
#include <map>
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

    struct QuantClass {
        bool grouping;
        uint16_t bits_per_codeword;
    };

    std::map<uint32_t, QuantClass> QuantClasses = {
        {3, {true, 5}},
        {5, {true, 7}},
        {7, {false, 3}},
        {9, {true, 10}},
        {15, {false, 4}},
        {31, {false, 5}},
        {63, {false, 6}},
        {127, {false, 7}},
        {255, {false, 8}},
        {511, {false, 9}},
        {1023, {false, 10}},
        {2047, {false, 11}},
        {4095, {false, 12}},
        {8191, {false, 13}},
        {16383, {false, 14}},
        {32767, {false, 15}},
        {65535, {false, 16}}
    };
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

    ChannelValues allocation{};
    for (size_t sb = 0; sb < SBLIMIT; sb++) {
        auto nbal = Bits_For_Subband_B2a[sb];
        allocation[0][sb] = Level_For_Index_Subband[sb][data.read_bits_be(nbal)];
        allocation[1][sb] = Level_For_Index_Subband[sb][data.read_bits_be(nbal)];
    }

    return allocation;
}

mpeg1_aud::ChannelValues mpeg1_aud::read_scfsi(util::bitspan& data, ChannelValues& allocations) {
    ChannelValues scfsi{};

    for (size_t sb = 0; sb < SBLIMIT; sb++) {
        for (size_t ch = 0; ch < 2; ch++) {
            if (allocations[ch][sb]) {
                scfsi[ch][sb] = data.read_bits_be(2);
            }
        }
    }

    return scfsi;
}

mpeg1_aud::ScaleFactors mpeg1_aud::read_scale_factors(util::bitspan& data, ChannelValues& allocations, ChannelValues& scfsi) {
    ScaleFactors scalefactors{};

    for (size_t sb = 0; sb < SBLIMIT; sb++) {
        for (size_t ch = 0; ch < 2; ch++) {
            if (allocations[ch][sb]) {
                if (scfsi[ch][sb] == 0) {
                    scalefactors[ch][sb][0] = data.read_bits_be(6);
                    scalefactors[ch][sb][1] = data.read_bits_be(6);
                    scalefactors[ch][sb][2] = data.read_bits_be(6);
                } else if (scfsi[ch][sb] == 1) {
                    scalefactors[ch][sb][0] = data.read_bits_be(6);
                    scalefactors[ch][sb][1] = scalefactors[ch][sb][0];
                    scalefactors[ch][sb][2] = data.read_bits_be(6);
                } else if (scfsi[ch][sb] == 2) {
                    scalefactors[ch][sb][0] = data.read_bits_be(6);
                    scalefactors[ch][sb][1] = scalefactors[ch][sb][0];
                    scalefactors[ch][sb][2] = scalefactors[ch][sb][0];
                } else if (scfsi[ch][sb] == 3) {
                    scalefactors[ch][sb][0] = data.read_bits_be(6);
                    scalefactors[ch][sb][1] = data.read_bits_be(6);
                    scalefactors[ch][sb][2] = scalefactors[ch][sb][1];
                }
            }
        }
    }

    return scalefactors;
}

std::array<int, 3> mpeg1_aud::read_samples(util::bitspan& data, uint32_t level, uint32_t scale_factor) {
    if (level == 0) {
        return std::array<int, 3>{};
    }

    if (scale_factor == 63) {
        scale_factor = 0;
    } else {
        static const int scf_base[3] = { 0x02000000, 0x01965FEA, 0x01428A30 };
        auto sf_by_3 = scale_factor / 3;
        scale_factor = ((scf_base[scale_factor % 3] + (((1 << sf_by_3) >> 1))) >> sf_by_3);
    }

    std::array<int, 3> samples;

    auto quant_class = QuantClasses[level];
    if (quant_class.grouping) {
        auto val = data.read_bits_be(quant_class.bits_per_codeword);
        samples[0] = val % level;
        val /= level;
        samples[1] = val % level;
        samples[2] = val / level;
    } else {
        samples[0] = data.read_bits_be(quant_class.bits_per_codeword);
        samples[1] = data.read_bits_be(quant_class.bits_per_codeword);
        samples[2] = data.read_bits_be(quant_class.bits_per_codeword);
    }

    uint32_t scale = 65536 / (level + 1);
    level = ((level + 1) >> 1) - 1;

    for (size_t i = 0; i < 3; i++) {
        auto val = (level - samples[i]) * scale;
        samples[i] = (val * (scale_factor >> 12) +
                     ((val * (scale_factor & 4095) + 2048) >> 12))
                     >> 12;
    }

    return samples;
}

namespace {
    std::array<std::array<int, 32>, 64> gen_nik_table() {
        std::array<std::array<int, 32>, 64> nik{};
        for (size_t i = 0;  i < 64;  ++i)
            for (size_t j = 0;  j < 32;  ++j)
                nik[i][j] = (int) (256.0 * std::cos(((16 + i) * ((j << 1) + 1)) * 0.0490873852123405));

        return nik;
    }

    static int Voffs = 0;
    static std::array<std::array<int, 32>, 64> N = gen_nik_table();

    // synthesis window
    const std::array<int, 512> D = {
        0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,-0x00001,
        -0x00001,-0x00001,-0x00001,-0x00002,-0x00002,-0x00003,-0x00003,-0x00004,
        -0x00004,-0x00005,-0x00006,-0x00006,-0x00007,-0x00008,-0x00009,-0x0000A,
        -0x0000C,-0x0000D,-0x0000F,-0x00010,-0x00012,-0x00014,-0x00017,-0x00019,
        -0x0001C,-0x0001E,-0x00022,-0x00025,-0x00028,-0x0002C,-0x00030,-0x00034,
        -0x00039,-0x0003E,-0x00043,-0x00048,-0x0004E,-0x00054,-0x0005A,-0x00060,
        -0x00067,-0x0006E,-0x00074,-0x0007C,-0x00083,-0x0008A,-0x00092,-0x00099,
        -0x000A0,-0x000A8,-0x000AF,-0x000B6,-0x000BD,-0x000C3,-0x000C9,-0x000CF,
        0x000D5, 0x000DA, 0x000DE, 0x000E1, 0x000E3, 0x000E4, 0x000E4, 0x000E3,
        0x000E0, 0x000DD, 0x000D7, 0x000D0, 0x000C8, 0x000BD, 0x000B1, 0x000A3,
        0x00092, 0x0007F, 0x0006A, 0x00053, 0x00039, 0x0001D,-0x00001,-0x00023,
        -0x00047,-0x0006E,-0x00098,-0x000C4,-0x000F3,-0x00125,-0x0015A,-0x00190,
        -0x001CA,-0x00206,-0x00244,-0x00284,-0x002C6,-0x0030A,-0x0034F,-0x00396,
        -0x003DE,-0x00427,-0x00470,-0x004B9,-0x00502,-0x0054B,-0x00593,-0x005D9,
        -0x0061E,-0x00661,-0x006A1,-0x006DE,-0x00718,-0x0074D,-0x0077E,-0x007A9,
        -0x007D0,-0x007EF,-0x00808,-0x0081A,-0x00824,-0x00826,-0x0081F,-0x0080E,
        0x007F5, 0x007D0, 0x007A0, 0x00765, 0x0071E, 0x006CB, 0x0066C, 0x005FF,
        0x00586, 0x00500, 0x0046B, 0x003CA, 0x0031A, 0x0025D, 0x00192, 0x000B9,
        -0x0002C,-0x0011F,-0x00220,-0x0032D,-0x00446,-0x0056B,-0x0069B,-0x007D5,
        -0x00919,-0x00A66,-0x00BBB,-0x00D16,-0x00E78,-0x00FDE,-0x01148,-0x012B3,
        -0x01420,-0x0158C,-0x016F6,-0x0185C,-0x019BC,-0x01B16,-0x01C66,-0x01DAC,
        -0x01EE5,-0x02010,-0x0212A,-0x02232,-0x02325,-0x02402,-0x024C7,-0x02570,
        -0x025FE,-0x0266D,-0x026BB,-0x026E6,-0x026ED,-0x026CE,-0x02686,-0x02615,
        -0x02577,-0x024AC,-0x023B2,-0x02287,-0x0212B,-0x01F9B,-0x01DD7,-0x01BDD,
        0x019AE, 0x01747, 0x014A8, 0x011D1, 0x00EC0, 0x00B77, 0x007F5, 0x0043A,
        0x00046,-0x003E5,-0x00849,-0x00CE3,-0x011B4,-0x016B9,-0x01BF1,-0x0215B,
        -0x026F6,-0x02CBE,-0x032B3,-0x038D3,-0x03F1A,-0x04586,-0x04C15,-0x052C4,
        -0x05990,-0x06075,-0x06771,-0x06E80,-0x0759F,-0x07CCA,-0x083FE,-0x08B37,
        -0x09270,-0x099A7,-0x0A0D7,-0x0A7FD,-0x0AF14,-0x0B618,-0x0BD05,-0x0C3D8,
        -0x0CA8C,-0x0D11D,-0x0D789,-0x0DDC9,-0x0E3DC,-0x0E9BD,-0x0EF68,-0x0F4DB,
        -0x0FA12,-0x0FF09,-0x103BD,-0x1082C,-0x10C53,-0x1102E,-0x113BD,-0x116FB,
        -0x119E8,-0x11C82,-0x11EC6,-0x120B3,-0x12248,-0x12385,-0x12467,-0x124EF,
        0x1251E, 0x124F0, 0x12468, 0x12386, 0x12249, 0x120B4, 0x11EC7, 0x11C83,
        0x119E9, 0x116FC, 0x113BE, 0x1102F, 0x10C54, 0x1082D, 0x103BE, 0x0FF0A,
        0x0FA13, 0x0F4DC, 0x0EF69, 0x0E9BE, 0x0E3DD, 0x0DDCA, 0x0D78A, 0x0D11E,
        0x0CA8D, 0x0C3D9, 0x0BD06, 0x0B619, 0x0AF15, 0x0A7FE, 0x0A0D8, 0x099A8,
        0x09271, 0x08B38, 0x083FF, 0x07CCB, 0x075A0, 0x06E81, 0x06772, 0x06076,
        0x05991, 0x052C5, 0x04C16, 0x04587, 0x03F1B, 0x038D4, 0x032B4, 0x02CBF,
        0x026F7, 0x0215C, 0x01BF2, 0x016BA, 0x011B5, 0x00CE4, 0x0084A, 0x003E6,
        -0x00045,-0x00439,-0x007F4,-0x00B76,-0x00EBF,-0x011D0,-0x014A7,-0x01746,
        0x019AE, 0x01BDE, 0x01DD8, 0x01F9C, 0x0212C, 0x02288, 0x023B3, 0x024AD,
        0x02578, 0x02616, 0x02687, 0x026CF, 0x026EE, 0x026E7, 0x026BC, 0x0266E,
        0x025FF, 0x02571, 0x024C8, 0x02403, 0x02326, 0x02233, 0x0212B, 0x02011,
        0x01EE6, 0x01DAD, 0x01C67, 0x01B17, 0x019BD, 0x0185D, 0x016F7, 0x0158D,
        0x01421, 0x012B4, 0x01149, 0x00FDF, 0x00E79, 0x00D17, 0x00BBC, 0x00A67,
        0x0091A, 0x007D6, 0x0069C, 0x0056C, 0x00447, 0x0032E, 0x00221, 0x00120,
        0x0002D,-0x000B8,-0x00191,-0x0025C,-0x00319,-0x003C9,-0x0046A,-0x004FF,
        -0x00585,-0x005FE,-0x0066B,-0x006CA,-0x0071D,-0x00764,-0x0079F,-0x007CF,
        0x007F5, 0x0080F, 0x00820, 0x00827, 0x00825, 0x0081B, 0x00809, 0x007F0,
        0x007D1, 0x007AA, 0x0077F, 0x0074E, 0x00719, 0x006DF, 0x006A2, 0x00662,
        0x0061F, 0x005DA, 0x00594, 0x0054C, 0x00503, 0x004BA, 0x00471, 0x00428,
        0x003DF, 0x00397, 0x00350, 0x0030B, 0x002C7, 0x00285, 0x00245, 0x00207,
        0x001CB, 0x00191, 0x0015B, 0x00126, 0x000F4, 0x000C5, 0x00099, 0x0006F,
        0x00048, 0x00024, 0x00002,-0x0001C,-0x00038,-0x00052,-0x00069,-0x0007E,
        -0x00091,-0x000A2,-0x000B0,-0x000BC,-0x000C7,-0x000CF,-0x000D6,-0x000DC,
        -0x000DF,-0x000E2,-0x000E3,-0x000E3,-0x000E2,-0x000E0,-0x000DD,-0x000D9,
        0x000D5, 0x000D0, 0x000CA, 0x000C4, 0x000BE, 0x000B7, 0x000B0, 0x000A9,
        0x000A1, 0x0009A, 0x00093, 0x0008B, 0x00084, 0x0007D, 0x00075, 0x0006F,
        0x00068, 0x00061, 0x0005B, 0x00055, 0x0004F, 0x00049, 0x00044, 0x0003F,
        0x0003A, 0x00035, 0x00031, 0x0002D, 0x00029, 0x00026, 0x00023, 0x0001F,
        0x0001D, 0x0001A, 0x00018, 0x00015, 0x00013, 0x00011, 0x00010, 0x0000E,
        0x0000D, 0x0000B, 0x0000A, 0x00009, 0x00008, 0x00007, 0x00007, 0x00006,
        0x00005, 0x00005, 0x00004, 0x00004, 0x00003, 0x00003, 0x00002, 0x00002,
        0x00002, 0x00002, 0x00001, 0x00001, 0x00001, 0x00001, 0x00001, 0x00001
    };
}

mpeg1_aud::DecodedSamples mpeg1_aud::decode_samples(util::bitspan& data, ChannelValues& allocations, ScaleFactors& scale_factors) {

    DecodedSamples decoded;
    short* decoded_ptr = &decoded[0];
    mpeg1_aud::Samples samp{};
    std::array<std::array<int, 1024>, 2> V{};

    //scfsi[sb] states frame is divided into 3 equal prats of 12 subband samples
    for (size_t part = 0; part < 3; part++) {
        for (size_t gr = 0; gr < 4; gr++) { // four granules per part
            for (size_t sb = 0; sb < SBLIMIT; sb++) {
                for (size_t ch = 0; ch < 2; ch++) {
                    samp[ch][sb] = read_samples(data, allocations[ch][sb], scale_factors[ch][sb][part]);
                }
            }

            for (size_t sIdx = 0; sIdx < 3; sIdx++) { // sample index for each in the granule
                Voffs = (Voffs - 64) & 1024;
                auto table_idx = Voffs;

                for (size_t ch = 0; ch < 2; ch++) {
                    for (size_t i = 0; i < 64; i++) {
                        auto sum = 0;
                        for (size_t k = 0; k < 32; k++) {
                            sum += N[i][k] * samp[ch][k][sIdx];
                        }
                        V[ch][table_idx + i] = (sum + 8192) >> 14;
                    }

                    std::array<int, 512> U{};
                    for (size_t i = 0; i < 8; i++) {
                        for (size_t j = 0; j < 32; j++) {
                            U[i * 64 + j] = V[ch][(table_idx + i * 128 + j) & 1023];
                            U[i * 64 + 32 + j] = V[ch][(table_idx + i * 128 + 96 + j) & 1023];
                        }
                    }

                    for (size_t i = 0; i < 512; i++) {
                        U[i] = ((U[i] * D[i]) + 32) >> 6;
                    }

                    for (size_t j = 0; j < 32; j++) {
                        auto sum = 0;
                        for (size_t i = 0; i < 16; i++) {
                            sum -= U[j + 32 * i];
                        }
                        sum = std::clamp(sum, -32768, 32767);
                        decoded_ptr[(sIdx << 6) | (j << 1) | ch] = sum;
                    }
                }
            }
            decoded_ptr += 96;
        }
    }

    return decoded;
}