
//------------------------------------------------------------------------------
// mpegts.tests.cpp
//------------------------------------------------------------------------------

#include "mpegts.hpp"

#include <gtest/gtest.h>

namespace {

// https://stackoverflow.com/a/45172360
template<typename... Ts>
std::array<std::byte, sizeof...(Ts)> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

}

TEST(MPEGTS, read_header) {
    auto data = make_bytes(0x47, 0x40, 0x11, 0x10);
    auto span = std::span<std::byte>(data);
    auto header = pg1::read_ts_pkt_header(span);

    EXPECT_EQ(header.tei, false);
    EXPECT_EQ(header.pusi, true);
    EXPECT_EQ(header.priority, false);
    EXPECT_EQ(header.pid, 17);
    EXPECT_EQ(header.tsc, 0);
    EXPECT_EQ(header.afc, 1);
    EXPECT_EQ(header.continuity_counter, 0);

    EXPECT_TRUE(span.empty());
}