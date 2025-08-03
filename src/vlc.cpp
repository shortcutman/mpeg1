
//------------------------------------------------------------------------------
// vlc.cpp
//------------------------------------------------------------------------------

#include "vlc.hpp"

#include <algorithm>
#include <cassert>

mpeg1::VariableLengthCode::VariableLengthCode(std::vector<HuffmanCode> codes) {
    _codes = std::move(codes);

    std::sort(_codes.begin(), _codes.end());
}

size_t mpeg1::VariableLengthCode::next_symbol(util::bitspan& data) const {
    assert(std::is_sorted(this->_codes.begin(), this->_codes.end()));

    for (auto& code : this->_codes) {
        auto bits = data.peek_bits_be(code.code_length);
        if (bits == code.code) {
            data.read_bits_be(code.code_length);
            return code.symbol;
        }
    }

    throw std::runtime_error("Couldn't find a matching code.");

    return 0;
}