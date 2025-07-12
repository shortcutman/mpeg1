
//------------------------------------------------------------------------------
// mpegts.hpp
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <span>


namespace pg1 {

    struct TSHeader {
        bool tei;
        bool pusi;
        bool priority;
        uint16_t pid;
        uint8_t tsc;
        uint8_t afc;
        uint8_t continuity_counter;
    };

    TSHeader read_ts_pkt_header(std::span<std::byte>& data);

    void loop_ts_data(std::span<std::byte>& data);
}
