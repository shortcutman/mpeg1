
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

