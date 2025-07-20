
//------------------------------------------------------------------------------
// bitspan.hpp
//------------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace util {
    class bitspan {
    private:
        std::span<std::byte> _data;
        size_t _bit_offset;

    public:
        bitspan(std::span<std::byte> data);
        bitspan(std::span<std::byte> data, size_t bit_offset);
        bitspan(bitspan& data);

        template<std::size_t N>
        bitspan(std::array<std::byte, N>& arr) : bitspan(std::span{arr}) {}

        size_t bits_read() const;
        size_t bytes_read() const;

        uint32_t peek_bits_le(uint8_t bits);
        uint32_t read_bits_le(uint8_t bits);

        uint32_t peek_bits_be(uint8_t bits);
        uint32_t read_bits_be(uint8_t bits);

        void round_to_next_byte();

        std::span<std::byte> to_span() const;
    };
}