
//------------------------------------------------------------------------------
// decoder.cpp
//------------------------------------------------------------------------------

#include "decoder.hpp"

#include "mpeg1.hpp"

#include <print>
#include <span>

void mpeg1::Decoder::set_data(Data data) {
    _data = data;
}

std::expected<mpeg1::Decoder::Frame, std::runtime_error> mpeg1::Decoder::next_frame() {
    if (_data.empty()) {
        return std::unexpected(std::runtime_error("No bytes to parse."));
    }

    auto next = next_code();
    if (!next) {
        std::println("Decoder error: {}", next.error().what());
        return Frame();
    }

    auto [code, bytes] = next.value();

    std::println("Code: {}", code);

    return Frame();
}

bool mpeg1::Decoder::peak_code() const {
    return _data.size() >= 4 &&
           _data[0] == std::byte{0x00} &&
           _data[1] == std::byte{0x00} &&
           _data[2] == std::byte{0x01};
}

std::expected<std::tuple<uint32_t, std::span<std::byte>>, std::runtime_error>
    mpeg1::Decoder::next_code() {

    for (size_t i = 0; i < _data.size(); i++) {
        if (peak_code()) {
            uint32_t code = 0;
            code |= 0x0100 | std::to_integer<uint8_t>(_data[3]);
            return std::make_tuple(code, _data);
        }
    }

    return std::unexpected(std::runtime_error("Could not find start code."));
}
