
//------------------------------------------------------------------------------
// decoder.hpp
//------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <vector>

namespace image {
    class Colour;
}

namespace mpeg1 {
    class Decoder {
    public:
        typedef std::vector<std::byte> Data;
        typedef std::vector<image::Colour> Frame;
        typedef std::function<void (const Frame&)> FrameCallback;

    private:
        Data* _data;
        FrameCallback _frame_callback;

    public:
        Decoder() {}
        ~Decoder() {}

        void set_data(Data* data);
        void set_callback(FrameCallback callback);
    };
}
