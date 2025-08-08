
//------------------------------------------------------------------------------
// mpeg1.vlc.hpp
//------------------------------------------------------------------------------

#pragma once

#include "vlc.hpp"

namespace mpeg1 {

// Table B.1 Variable length codes for macroblock_address_increment
const VariableLengthCode<size_t> MACROBLOCK_ADDRESSING = {{
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

struct MacroblockType {
    bool quant;
    bool motion_forward;
    bool motion_backward;
    bool pattern;
    bool intra;
};

// Table 5.2b Variable length codes for macroblock_type in intra-coded pictures.
const VariableLengthCode<MacroblockType> MACROBLOCK_TYPE_INTRA_VLC = {{
    {0b1, 1, {false, false, false, false, true}},
    {0b01, 2, {true, false, false, false, true}}
}};

// Table B5.a Variable length codes for dct_dc_size_luminance
const VariableLengthCode<size_t> BLOCK_DCT_DC_SIZE_LUMINANCE = {{
    {0b100, 3, 0},
    {0b00, 2, 1},
    {0b01, 2, 2},
    {0b101, 3, 4},
    {0b1110, 4, 5},
    {0b11110, 5, 6},
    {0b111110, 6, 7},
    {0b1111110, 7, 8},
}};

// Table B5.b Variable length codes for dct_dc_size_chrominance
const VariableLengthCode<size_t> BLOCK_DCT_DC_SIZE_CHROMINANCE = {{
    {0b00, 2, 0},
    {0b01, 2, 1},
    {0b10, 2, 2},
    {0b110, 3, 3},
    {0b1110, 4, 4},
    {0b11110, 5, 5},
    {0b111110, 6, 6},
    {0b1111110, 7, 7},
    {0b11111110, 8, 8},
}};

struct DCTCoeff {
    int8_t run;
    int8_t level;
};


const VariableLengthCode<DCTCoeff> BLOCK_DCT_COEFF = {{
// Table B.5c Variable length codes for dct_coeff_first and dct_coeff_next
    {0b1, 1, {0, 1}},

    {0b11, 2, {0, 1}},

    {0b011, 3, {1, 1}},

    {0b0100, 4, {0, 2}},
    {0b0101, 4, {2, 1}},

    {0b00101, 5, {0, 3}},
    {0b00111, 5, {3, 1}},
    {0b00110, 5, {4, 1}},

    {0b000110, 6, {1, 2}},
    {0b000111, 6, {5, 1}},
    {0b000101, 6, {6, 1}},
    {0b000100, 6, {7, 1}},

    {0b0000110, 7, {0, 4}},
    {0b0000100, 7, {2, 2}},
    {0b0000111, 7, {8, 1}},
    {0b0000101, 7, {9, 1}},

    {0b00100110, 8, {0, 5}},
    {0b00100001, 8, {0, 6}},
    {0b00100101, 8, {1, 3}},
    {0b00100100, 8, {3, 2}},
    {0b00100111, 8, {10, 1}},
    {0b00100011, 8, {11, 1}},
    {0b00100010, 8, {12, 1}},
    {0b00100000, 8, {13, 1}},

    {0b0000001010, 10, {0, 7}},
    {0b0000001100, 10, {1, 4}},
    {0b0000001011, 10, {2, 3}},
    {0b0000001111, 10, {4, 2}},
    {0b0000001001, 10, {5, 2}},
    {0b0000001110, 10, {14, 1}},
    {0b0000001101, 10, {15, 1}},
    {0b0000001000, 10, {16, 1}},
}};

}
