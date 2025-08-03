
//------------------------------------------------------------------------------
// vlc.hpp
//------------------------------------------------------------------------------

#pragma once

#include "bitspan.hpp"

#include <cstdint>
#include <vector>

namespace mpeg1 {

class VariableLengthCode {

    public:
        struct HuffmanCode {
            size_t code;
            size_t code_length;
            size_t symbol;

            bool operator==(const HuffmanCode& a) const = default;
            bool operator<(const HuffmanCode& b) const {
                return this->code_length < b.code_length;
            }
        };

    private:
        std::vector<HuffmanCode> _codes;

    public:
        VariableLengthCode(std::vector<HuffmanCode> codes);
        ~VariableLengthCode() {}

        size_t next_symbol(util::bitspan& data) const;
};

const VariableLengthCode MACROBLOCK_ADDRESSING = {{
    {0b1, 1, 1},
    {0b011, 3, 2},
    {0b010, 3, 3},
    {0b0011, 4, 4},
    {0b0010, 4, 5},
    {0b00011, 5, 6},
    {0b00010, 5, 7},
    {0b0000111, 7, 8},
    {0b0000110, 7, 9},
    {0b00001011, 8, 10},
    {0b00001010, 8, 11},
    {0b00001001, 8, 12},
    {0b00001000, 8, 13},
    {0b00000111, 8, 14},
    {0b00000110, 8, 15},
    {0b0000010111, 10, 16},
    {0b0000010110, 10, 17},
    {0b0000010101, 10, 18},
    {0b0000010100, 10, 19},
    {0b0000010011, 10, 20},
    {0b0000010010, 10, 21},
    {0b00000100011, 11, 22},
    {0b00000100010, 11, 23},
    {0b00000100001, 11, 24},
    {0b00000100000, 11, 25},
    {0b00000011111, 11, 26},
    {0b00000011110, 11, 27},
    {0b00000011101, 11, 28},
    {0b00000011100, 11, 29},
    {0b00000011011, 11, 30},
    {0b00000011010, 11, 31},
    {0b00000011001, 11, 32},
    {0b00000011000, 11, 33},
}};

}
