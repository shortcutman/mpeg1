
//------------------------------------------------------------------------------
// mpegts.cpp
//------------------------------------------------------------------------------

#include "mpegts.hpp"

#include "bitspan.hpp"

#include <map>
#include <print>

pg1::TSHeader pg1::read_ts_pkt_header(std::span<std::byte>& data) {
    TSHeader header;

    util::bitspan bits(data);

    if (bits.read_bits_be(8) != 0x47) {
        throw std::runtime_error("Expected MPEG2TS Sync Byte");
    }

    header.tei = bits.read_bits_be(1);
    header.pusi = bits.read_bits_be(1);
    header.priority = bits.read_bits_be(1);
    header.pid = bits.read_bits_be(13);
    header.tsc = bits.read_bits_be(2);
    header.afc = bits.read_bits_be(2);
    header.continuity_counter = bits.read_bits_be(4);

    data = data.subspan(4);

    return header;
}

pg1::PSIHeader pg1::read_psi_header(std::span<std::byte>& data) {
    PSIHeader header;

    util::bitspan bits(data);

    auto pointer_field = bits.read_bits_be(8);
    if (pointer_field != 0) {
        throw std::runtime_error("unhandled");
    }
    data = data.subspan(1);

    header.table_id = bits.read_bits_be(8); //table id
    header.section_syntax_indicator = bits.read_bits_be(1); //section syntax indicator
    bits.read_bits_be(1); //private bit
    bits.read_bits_be(2); //reserved
    bits.read_bits_be(2); //unused
    header.section_length = bits.read_bits_be(10); //section length

    data = data.subspan(3);

    if (header.section_syntax_indicator) {
        header.table_id_extension = bits.read_bits_be(16); //table id ext
        bits.read_bits_be(2); //reserved bits
        header.version_number = bits.read_bits_be(5); //version number
        bits.read_bits_be(1); //current/next indicator
        header.section_number = bits.read_bits_be(8); //section number
        header.last_section_number = bits.read_bits_be(8); //last section number
        data = data.subspan(5);
    }

    return header;
}


void pg1::loop_ts_data(std::span<std::byte>& data) {
    std::map<uint16_t, size_t> pid_map;

    while (!data.empty() || data.size() > 188) {
        auto header = read_ts_pkt_header(data);

        if (!pid_map.contains(header.pid)) {
            std::println("Found new PID: {}", header.pid);
            pid_map.insert({header.pid, 1});
        } else {
            pid_map[header.pid]++;
        }

        data = data.subspan(184);
    }

    std::println("Summary");
    for (auto& v : pid_map) {
        std::println("\tPID: {}, Packet count: {}", v.first, v.second);
    }
}

