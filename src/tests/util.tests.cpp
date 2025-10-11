
//------------------------------------------------------------------------------
// util.tests.cpp
//------------------------------------------------------------------------------

#include "util.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>

TEST(util, transform_out) {
    std::array<int, 8> input;
    std::array<bool, 16> output;

    output.fill(false);

    util::transform_out(input.begin(), input.end(), output.begin(),
        [] (const int val, bool& out) {
            out = true;
        });

    EXPECT_TRUE(std::all_of(output.begin(), output.begin() + 8, [] (bool i) {return i == true;}));
    EXPECT_TRUE(std::all_of(output.begin() + 8, output.end(), [] (bool i) {return i == false;}));
}
