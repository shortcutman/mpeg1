
//------------------------------------------------------------------------------
// decoder.cpp
//------------------------------------------------------------------------------

#include "decoder.hpp"

void mpeg1::Decoder::set_data(std::vector<std::byte>* data) {
    _data = data;
}

void mpeg1::Decoder::set_callback(FrameCallback callback) {
    _frame_callback = callback;
}
