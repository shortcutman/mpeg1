
//------------------------------------------------------------------------------
// util.cpp
//------------------------------------------------------------------------------

#include "util.hpp"

#include "bitspan.hpp"

#include <algorithm>
#include <print>

util::ByteCatcha::ByteCatcha(util::bitspan& data)
: _bits_read_start(data.bits_read())
, _data(data.to_span())
, _bits(data)
{
}

util::ByteCatcha::~ByteCatcha() {
    size_t bytes = std::max((_bits.bits_read() - _bits_read_start) / 8, (size_t)3);
    std::println("ByteCatcha, read {} bits at start, full bytes:", _bits_read_start % 8);
    std::for_each_n(_data.begin(), bytes, [] (auto b) {
        std::print("0x{:02x}, ", std::to_integer<int>(b));
    });
    std::println();
}
