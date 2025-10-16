
//------------------------------------------------------------------------------
// bitspan.cpp
//------------------------------------------------------------------------------

#include "bitspan.hpp"

#include <cstddef>

util::bitspan::bitspan(std::span<std::byte> data)
    : _data(data)
    , _bit_offset(0) {
}

util::bitspan::bitspan(std::span<std::byte> data, size_t bit_offset)
    : _data(data)
    , _bit_offset(bit_offset) {
}

util::bitspan::bitspan(bitspan& data) 
    : _data(data._data) 
    , _bit_offset(data._bit_offset) {
}

size_t util::bitspan::bits_read() const {
    return _bit_offset;
}

size_t util::bitspan::bytes_read() const {
    auto bytes = _bit_offset / 8;
    bytes += _bit_offset % 8 ? 1 : 0;
    return bytes;
}

uint32_t util::bitspan::peek_bits_le(uint8_t bits) const {
    if (bits == 0) {
        return 0;
    }

    int64_t bits_remain = static_cast<int64_t>(_data.size() * 8) - _bit_offset - bits;
    if (bits_remain < 0) {
        throw std::runtime_error("Not enough bits available.");
    }

    size_t byte_offset = _bit_offset / 8;
    size_t bits_in = _bit_offset % 8;

    uint32_t val = *reinterpret_cast<uint32_t*>(&_data[byte_offset]);

    val >>= bits_in;
    //mask for bits asked for
    val &= 0xffffffff >> (32 - bits);

    return val;
}

uint32_t util::bitspan::peek_bits_be(uint8_t bits) const {
    if (bits == 0) {
        return 0;
    }

    int64_t bits_remain = static_cast<int64_t>(_data.size() * 8) - _bit_offset - bits;
    if (bits_remain < 0) {
        throw std::runtime_error("Not enough bits available.");
    }

    size_t byte_offset = _bit_offset / 8;
    size_t bits_in = _bit_offset % 8;

    uint32_t val = 0;
    val |= *reinterpret_cast<uint8_t*>(&_data[byte_offset]) << 24;
    val |= *reinterpret_cast<uint8_t*>(&_data[byte_offset + 1]) << 16;
    val |= *reinterpret_cast<uint8_t*>(&_data[byte_offset + 2]) << 8;
    val |= *reinterpret_cast<uint8_t*>(&_data[byte_offset + 3]);

    val <<= bits_in;
    //mask for bits asked for
    // val &= (0xffffffff >> (32 - bits));
    val >>= (32 - bits);

    return val;
}

uint32_t util::bitspan::read_bits_le(uint8_t bits) {
    auto ret = peek_bits_le(bits);
    _bit_offset += bits;
    return ret;
}

uint32_t util::bitspan::read_bits_be(uint8_t bits) {
    auto ret = peek_bits_be(bits);
    _bit_offset += bits;
    return ret;
}

void util::bitspan::round_to_next_byte() {
    _bit_offset += _bit_offset % 8;
}

std::span<std::byte> util::bitspan::to_span() const {
    auto bytes_in = _bit_offset / 8;
    return std::span{&_data[bytes_in], _data.size() - bytes_in};
}

std::span<std::byte> util::bitspan::to_aligned_span() const {
    auto bytes_in = this->bytes_read();
    return std::span{&_data[bytes_in], _data.size() - bytes_in};
}
