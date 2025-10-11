
//------------------------------------------------------------------------------
// idct.hpp
//------------------------------------------------------------------------------

#pragma once

#include <array>

namespace image {

typedef std::array<int, 8*8> DataUnit;

DataUnit idct_float(const DataUnit& du);
DataUnit idct_float_table(const DataUnit& du);
DataUnit dct_float_loeffler(const DataUnit& du);
void idct_float_loeffler(DataUnit& du);
DataUnit idct_int(const DataUnit& du);
DataUnit idct_int_table(const DataUnit& du);

#define idct idct_float_loeffler

std::array<int, 8> loeffler_1d_dct(const std::array<int, 8> in);

inline void loeffler_1d_idct_row(const std::array<int, 8*8>& du, std::array<float, 8*8>& inter, int offset);
inline void loeffler_1d_idct_col(const std::array<float, 8*8>& du, std::array<int, 8*8>& out, int offset);


}
