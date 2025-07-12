
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

TEST(MPEGTS, read_ts_header) {
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

TEST(MPEGTS, read_psi_header_pat) {
    auto data = make_bytes(0x00, 0x00, 0xB0, 0x0D, 0x00, 0x01, 0xC1, 0x00, 0x00);
    auto span = std::span<std::byte>(data);
    auto header = pg1::read_psi_header(span);

    EXPECT_EQ(header.table_id, 0);
    EXPECT_EQ(header.section_syntax_indicator, true);
    EXPECT_EQ(header.section_length, 13);
    EXPECT_EQ(header.table_id_extension, 1);
    EXPECT_EQ(header.version_number, 0);
    EXPECT_EQ(header.section_number, 0);
    EXPECT_EQ(header.last_section_number, 0);

    EXPECT_TRUE(span.empty());
}

TEST(MPEGTS, read_psi_header_pmt) {
    auto data = make_bytes(0x00, 0x02, 0xB0, 0x1D, 0x00, 0x01, 0xC1, 0x00, 0x00);
    auto span = std::span<std::byte>(data);
    auto header = pg1::read_psi_header(span);

    EXPECT_EQ(header.table_id, 2);
    EXPECT_EQ(header.section_syntax_indicator, true);
    EXPECT_EQ(header.section_length, 29);
    EXPECT_EQ(header.table_id_extension, 1);
    EXPECT_EQ(header.version_number, 0);
    EXPECT_EQ(header.section_number, 0);
    EXPECT_EQ(header.last_section_number, 0);

    EXPECT_TRUE(span.empty());
}