
//------------------------------------------------------------------------------
// mpegts.cpp
//------------------------------------------------------------------------------

#include "mpegts.hpp"

#include "bitspan.hpp"

#include <format>
#include <map>
#include <optional>
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

void pg1::read_adaption_field(std::span<std::byte>& data) {
    util::bitspan bits(data);
    auto length = bits.read_bits_be(8);
    data = data.subspan(length + 1);
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

pg1::PAT pg1::read_pat_pkt(std::span<std::byte>& data) {
    PAT pat;

    if (data.size() < 184) {
        throw std::runtime_error("Expected TS Packet minus TS header.");
    }

    auto pkt_span = data.first(184);
    auto psi_header = read_psi_header(pkt_span);
    if (psi_header.table_id != 0) {
        throw std::runtime_error("Expected PSI table_id to equal 0 for PAT.");
    } else if (!psi_header.section_syntax_indicator) {
        throw std::runtime_error("Expected PSI section_syntax_indicator to be set.");
    }

    for (size_t programs = psi_header.section_length / 8; programs > 0; programs--) {
        PAT::Program program;
        util::bitspan bits(pkt_span);
        program.program_num = bits.read_bits_be(16);
        bits.read_bits_be(3);
        program.program_map_pid = bits.read_bits_be(13);

        pat.programs.push_back(program);
        pkt_span = pkt_span.subspan(4);
    }

    data = data.subspan(184);

    return pat;
}

pg1::PMT pg1::read_pmt_pkt(std::span<std::byte>& data) {
    PMT pmt;

    if (data.size() < 184) {
        throw std::runtime_error("Expected TS Packet minus TS header.");
    }

    auto pkt_span = data.first(184);
    auto psi_header = read_psi_header(pkt_span);
    if (psi_header.table_id != 2) {
        throw std::runtime_error("Expected PSI table_id to equal 2 for PMT.");
    } else if (!psi_header.section_syntax_indicator) {
        throw std::runtime_error("Expected PSI section_syntax_indicator to be set.");
    }

    util::bitspan bits(pkt_span);
    bits.read_bits_be(3);
    pmt.pcr_pid = bits.read_bits_be(13);
    bits.read_bits_be(4);
    auto program_info_length = bits.read_bits_be(12);
    pkt_span = pkt_span.subspan(4 + program_info_length);

    psi_header.section_length -= 5 + 4 + program_info_length;

    while (psi_header.section_length > 4) {
        PMT::ES es;
        util::bitspan bits(pkt_span);
        es.stream_type = bits.read_bits_be(8);
        bits.read_bits_be(3);
        es.elementary_pid = bits.read_bits_be(13);
        bits.read_bits_be(4);
        auto es_info_length = bits.read_bits_be(12);
        pkt_span = pkt_span.subspan(5 + es_info_length);
        psi_header.section_length -= 5 + es_info_length;
        pmt.elementary_streams.push_back(es);
    }

    data = data.subspan(184);

    return pmt;
}

void pg1::loop_ts_data(std::span<std::byte>& data, std::vector<std::byte>& video_es, std::vector<std::byte>& audio_es) {
    std::map<uint16_t, size_t> pid_map;
    std::optional<PAT> pat;
    std::optional<uint16_t> pmt_pid;
    bool displayed_pmt = false;
    std::optional<uint16_t> video_es_pid;
    std::optional<uint16_t> audio_es_pid;

    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] == std::byte{0x47}) {
            data = data.subspan(i);
            std::println("Found first 0x47 at byte no. {}", i);
            break;
        }
    }

    while (!data.empty() && data.size() > 188) {
        auto ts_pkt_span = data.first(188);

        auto header = read_ts_pkt_header(ts_pkt_span);

        if (header.afc & 0x2) {
            read_adaption_field(ts_pkt_span);
        }

        if (!pid_map.contains(header.pid)) {
            std::println("Found new PID: {}", header.pid);
            pid_map.insert({header.pid, 1});
        } else {
            pid_map[header.pid]++;
        }

        if (header.afc & 0x1) { //does adaption field indicate a payload?
            if (header.pid == 0 && !pat) {
                pat = read_pat_pkt(ts_pkt_span);
                pmt_pid = pat->programs.front().program_map_pid;
                std::println("PAT");
                std::println("\tPMT PID is: {}", *pmt_pid);
            } else if (pmt_pid && header.pid == *pmt_pid && !displayed_pmt) {
                displayed_pmt = true;
                auto pmt = read_pmt_pkt(ts_pkt_span);
                std::println("PMT:");
                std::println("\tPCR PID: {}", pmt.pcr_pid);

                for (auto es : pmt.elementary_streams) {
                    std::println("\tES Stream: {}, PID: {}", es.stream_type, es.elementary_pid);

                    if (es.stream_type == 2) {
                        video_es_pid = es.elementary_pid;
                        std::println("\tLogged as video ES.");
                    } else if (es.stream_type == 3) {
                        audio_es_pid = es.elementary_pid;
                        std::println("\tLogged as audio ES.");
                    }
                }
            } else if (video_es_pid && header.pid == *video_es_pid) {
                video_es.insert(video_es.end(), ts_pkt_span.begin(), ts_pkt_span.begin() + ts_pkt_span.size());
            } else if (audio_es_pid && header.pid == *audio_es_pid) {
                audio_es.insert(audio_es.end(), ts_pkt_span.begin(), ts_pkt_span.begin() + ts_pkt_span.size());
            }
        }

        data = data.subspan(188);
    }

    std::println("TS Packets found");
    for (auto& v : pid_map) {
        std::println("\tPID: {}, Count: {}", v.first, v.second);
        if (video_es_pid && v.first == *video_es_pid) {
            std::println("\t\tVideo bytes: {}", video_es.size());
        } else if (audio_es_pid && v.first == *audio_es_pid) {
            std::println("\t\tAudio bytes: {}", audio_es.size());
        }
    }

}
