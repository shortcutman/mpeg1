
//------------------------------------------------------------------------------
// mpeg1.tests.cpp
//------------------------------------------------------------------------------

#include "mpeg1.vlc.hpp"

#include <set>
#include <gtest/gtest.h>

TEST(VariableLengthTables, validate_BLOCK_DCT_COEFF) {
    std::set<mpeg1::DCTCoeff> coeffs;

    for (auto& c : mpeg1::BLOCK_DCT_COEFF_NEXT.codes()) {
        if (c.symbol == mpeg1::DCTCoeff{0, 1}) {
            continue;
        }

        EXPECT_FALSE(coeffs.contains(c.symbol))
            << "Repeat entry of run: " << static_cast<int>(c.symbol.run)
            << " and level: " << static_cast<int>(c.symbol.level);

        coeffs.insert(c.symbol);
    }
}
