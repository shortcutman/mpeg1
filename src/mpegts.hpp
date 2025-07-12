
//------------------------------------------------------------------------------
// mpegts.hpp
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <span>
#include <vector>

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

    struct PSIHeader {
        uint8_t table_id;
        bool section_syntax_indicator;
        uint16_t section_length;
        uint16_t table_id_extension;
        uint8_t version_number;
        uint8_t section_number;
        uint8_t last_section_number;
    };

    struct PAT {
        struct Program {
            uint16_t program_num;
            uint16_t program_map_pid;
        };

        std::vector<Program> programs;
    };

    TSHeader read_ts_pkt_header(std::span<std::byte>& data);

    PSIHeader read_psi_header(std::span<std::byte>& data);
    PAT read_pat_pkt(std::span<std::byte>& data);

    void loop_ts_data(std::span<std::byte>& data);
}
