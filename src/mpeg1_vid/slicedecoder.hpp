
//------------------------------------------------------------------------------
// slicedecoder.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"
#include "bitspan.hpp"
#include "mpeg1.hpp"

#include <span>

namespace mpeg1 {
    class SliceDecoder {
    private:
        mpeg1::SequenceHeader _sequence;
        mpeg1::PictureHeader _picture;
        mpeg1::SliceHeader _slice;

        int _dct_dc_y_past = 1024;
        int _dct_dc_cb_past = 1024;
        int _dct_dc_cr_past = 1024;

        int _past_intra_address = -2;
        int _mv_right_for_prev = 0;
        int _mv_down_for_prev = 0;

    public:
        SliceDecoder(mpeg1::SequenceHeader sequence, mpeg1::PictureHeader picture);

        int decode(std::span<std::byte>& data, const image::Frame& source, image::Frame& destination);

    protected:
        void reset();

        void read_intra_blocks(util::bitspan& data, int mb_addr, image::Frame& destination);
        std::array<int32_t, 8*8> read_intra_block(util::bitspan& data, int mb_addr, int block_num);
        std::array<int, 64> read_block(util::bitspan& data, size_t block_index);

        //for unit tests
        void set_slice(mpeg1::SliceHeader slice);
    };
}