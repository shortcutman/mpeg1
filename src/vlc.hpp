
//------------------------------------------------------------------------------
// vlc.hpp
//------------------------------------------------------------------------------

#pragma once

#include "bitspan.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

namespace mpeg1 {

template<typename VLCSymbol>
class VariableLengthCode {

    public:
        template<typename Symbol>
        struct HuffmanCode {
            size_t code;
            size_t code_length;
            Symbol symbol;

            bool operator==(const HuffmanCode& a) const = default;
            bool operator<(const HuffmanCode& b) const {
                return this->code_length < b.code_length;
            }
        };

    private:
        std::vector<HuffmanCode<VLCSymbol>> _codes;

    public:
        VariableLengthCode(std::vector<HuffmanCode<VLCSymbol>> codes);
        ~VariableLengthCode() {}

        VLCSymbol next_symbol(util::bitspan& data) const;
};

template<typename VLCSymbol>
mpeg1::VariableLengthCode<VLCSymbol>::VariableLengthCode(std::vector<HuffmanCode<VLCSymbol>> codes) {
    _codes = std::move(codes);

    std::sort(_codes.begin(), _codes.end());
}

template<typename VLCSymbol>
VLCSymbol mpeg1::VariableLengthCode<VLCSymbol>::next_symbol(util::bitspan& data) const {
    assert(std::is_sorted(this->_codes.begin(), this->_codes.end()));

    for (auto& code : this->_codes) {
        auto bits = data.peek_bits_be(code.code_length);
        if (bits == code.code) {
            data.read_bits_be(code.code_length);
            return code.symbol;
        }
    }

    throw std::runtime_error("Couldn't find a matching code.");

    return VLCSymbol{};
}


}


