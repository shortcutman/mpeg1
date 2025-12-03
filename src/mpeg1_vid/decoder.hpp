
//------------------------------------------------------------------------------
// decoder.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"

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

    public:
        Decoder() {}
        ~Decoder() {}

        void set_data(Data data);

        std::expected<Frame, std::runtime_error> next_frame();

    protected:
        bool peak_code() const;
        std::expected<std::tuple<uint32_t, std::span<std::byte>>, std::runtime_error> next_code();
    };
}
