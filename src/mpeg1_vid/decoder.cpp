
//------------------------------------------------------------------------------
// decoder.cpp
//------------------------------------------------------------------------------

#include "decoder.hpp"

#include "constants.hpp"
#include "copy.hpp"
#include "mpeg1.hpp"
#include "slicedecoder.hpp"

#include <print>
#include <span>

void mpeg1::Decoder::set_data(Data data) {
    _data = data;
}

std::expected<image::Frame, std::runtime_error> mpeg1::Decoder::next_frame() {
    if (_data.empty()) {
        return std::unexpected(std::runtime_error("No bytes to parse."));
    }

    while (!_data.empty()) {
        auto next = next_code();
        if (!next) {
            std::println("Decoder error: {}", next.error().what());
            return std::unexpected(next.error());
        }

        auto [code, bytes] = next.value();
        _data = bytes;

        if (code == mpeg1::start_code::sequence) {
            auto old_sequence = _sequence;
            _sequence = mpeg1::read_sequence_header(_data);

            if (_sequence.horizontal_size != old_sequence.horizontal_size ||
                _sequence.vertical_size != old_sequence.vertical_size) {
                auto pixel_count = _sequence.encoded_width() * _sequence.encoded_height();

                _last_frame.encoded_width = _sequence.encoded_width();
                _last_frame.encoded_height = _sequence.encoded_height();
                _last_frame.image.resize(pixel_count);

                _current_frame.encoded_width = _sequence.encoded_width();
                _current_frame.encoded_height = _sequence.encoded_height();
                _current_frame.image.resize(pixel_count);
            }
        } else if (code == mpeg1::start_code::group_of_pictures) {
            _gop = mpeg1::read_gop_header(_data);
        } else if (code == mpeg1::start_code::picture) {
            _picture = mpeg1::read_picture_header(_data);
        } else if (code >= mpeg1::start_code::slice_minimum &&
                   code <= mpeg1::start_code::slice_maximum) {
            mpeg1::SliceDecoder slicedecoder(_sequence, _picture);
            auto last_mb_addr = slicedecoder.decode(_data, _last_frame, _current_frame);

            if (last_mb_addr >= _sequence.mb_width() * _sequence.mb_height() - 1) {
                std::copy(_current_frame.image.begin(), _current_frame.image.end(), _last_frame.image.begin());

                auto imgcopy = _current_frame;
                for (auto& c : imgcopy.image) {
                    c = image::ycbcrToRGB(c);
                }
                return imgcopy;
            }
        } else {
            _data = _data.subspan(4);
        }
    }

    return image::Frame();
}

float mpeg1::Decoder::frame_rate() const {
    return _sequence.picture_rate;
}

bool mpeg1::Decoder::peak_code(size_t offset) const {
    return _data.size() - offset >= 4 &&
           _data[0 + offset] == std::byte{0x00} &&
           _data[1 + offset] == std::byte{0x00} &&
           _data[2 + offset] == std::byte{0x01};
}

bool mpeg1::Decoder::peak_code(util::bitspan& bits) const {
    auto unread_bytes_span = bits.to_aligned_span();
    return (unread_bytes_span[0] == std::byte{0x00} &&
            unread_bytes_span[1] == std::byte{0x00} &&
            unread_bytes_span[2] == std::byte{0x01});
}

std::expected<std::tuple<uint32_t, std::span<std::byte>>, std::runtime_error>
    mpeg1::Decoder::next_code() {

    for (size_t i = 0; i < _data.size(); i++) {
        if (peak_code(i)) {
            uint32_t code = 0;
            code |= 0x0100 | std::to_integer<uint8_t>(_data[3 + i]);
            return std::make_tuple(code, _data.subspan(i));
        }
    }

    return std::unexpected(std::runtime_error("Could not find start code."));
}
