
//------------------------------------------------------------------------------
// decoder.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"
#include "mpeg1.hpp"

#include <expected>
#include <functional>
#include <span>
#include <vector>

namespace mpeg1 {
    class Decoder {
    public:
        typedef std::span<std::byte> Data;
        typedef std::vector<image::Colour> Frame;

    private:
        Data _data;

        Frame _last_frame;
        Frame _current_frame;

        mpeg1::SequenceHeader _sequence;
        mpeg1::GroupOfPicturesHeader _gop;
        mpeg1::PictureHeader _picture;

        mpeg1::BlockContext _frame_context;

    public:
        Decoder() {}
        ~Decoder() {}

        void set_data(Data data);

        std::expected<Frame, std::runtime_error> next_frame();

    protected:
        bool peak_code(size_t offset) const;
        bool peak_code(util::bitspan& bits) const;
        std::expected<std::tuple<uint32_t, std::span<std::byte>>, std::runtime_error> next_code();


    };
}
