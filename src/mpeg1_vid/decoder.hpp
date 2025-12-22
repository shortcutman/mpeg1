
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

    private:
        Data _data;

        image::Frame _last_frame;
        image::Frame _current_frame;
        bool returned_current_frame = true;

        mpeg1::SequenceHeader _sequence;
        mpeg1::GroupOfPicturesHeader _gop;
        mpeg1::PictureHeader _picture;

    public:
        Decoder() {}
        ~Decoder() {}

        void set_data(Data data);

        std::expected<image::Frame, std::runtime_error> next_frame();

        float frame_rate() const;

    protected:
        bool peak_code(size_t offset) const;
        bool peak_code(util::bitspan& bits) const;
        std::expected<std::tuple<uint32_t, std::span<std::byte>>, std::runtime_error> next_code();

        image::Frame assemble_frame();
    };
}
