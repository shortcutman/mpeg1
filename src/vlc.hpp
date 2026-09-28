
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

template<typename VLCSymbol, int Size>
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

        typedef std::vector<HuffmanCode<VLCSymbol>> Codes;

    private:
        Codes _codes;
        std::array<HuffmanCode<VLCSymbol>*, 65556> _table = {};

    public:
        VariableLengthCode(std::vector<HuffmanCode<VLCSymbol>> codes);
        ~VariableLengthCode() {}

        VLCSymbol next_symbol(util::bitspan& data) const;
        VLCSymbol next_symbol_slow(util::bitspan& data) const;
        VLCSymbol next_symbol_opt(util::bitspan& data) const;

        const Codes& codes() const;
};

template<typename VLCSymbol, int Size>
mpeg1::VariableLengthCode<VLCSymbol, Size>::VariableLengthCode(std::vector<HuffmanCode<VLCSymbol>> codes) {
    _codes = std::move(codes);

    std::sort(_codes.begin(), _codes.end());

    std::vector<std::tuple<uint16_t, HuffmanCode<VLCSymbol>*>> adj;
    for (auto& a : _codes) {
        uint16_t val = a.code << (sizeof(uint16_t)*8 - a.code_length);
        adj.push_back(std::make_tuple(val, &a));
    }
    std::sort(adj.begin(), adj.end(), [](auto& a, auto&b) { return std::get<0>(a) < std::get<0>(b);});

    // check will be against highest (left) most bits
    for (auto& c : _codes) {
        size_t count = 1 << (16 - c.code_length);
        uint16_t adj = c.code << (16 - c.code_length);
        for (size_t i = 0; i < count; i++) {
                _table.at(adj | i) = &c;               
        }
    }
}

template<typename VLCSymbol, int Size>
VLCSymbol mpeg1::VariableLengthCode<VLCSymbol, Size>::next_symbol(util::bitspan& data) const {
    if constexpr (Size == 16) {
        return next_symbol_opt(data);
    } else {
        return next_symbol_slow(data);
    }
}

template<typename VLCSymbol, int Size>
VLCSymbol mpeg1::VariableLengthCode<VLCSymbol, Size>::next_symbol_slow(util::bitspan& data) const {
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

template<typename VLCSymbol, int Size>
VLCSymbol mpeg1::VariableLengthCode<VLCSymbol, Size>::next_symbol_opt(util::bitspan& data) const {
    auto bits = data.peak_16bits_be();

    auto result = _table[bits];
    if (result) {
        data.read_bits_be(result->code_length);
        return result->symbol;
    } else {
        throw std::runtime_error("Couldn't find a matching code.");
        return VLCSymbol{};
    }
}

template<typename VLCSymbol, int Size>
const VariableLengthCode<VLCSymbol, Size>::Codes& mpeg1::VariableLengthCode<VLCSymbol, Size>::codes() const {
    return _codes;
}

}
