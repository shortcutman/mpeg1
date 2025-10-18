
//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <iostream>

#include "mpegts.hpp"
#include "mpeg1_vid/decode.hpp"

namespace {

std::vector<std::byte> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("Args are wrong");
    }

    std::string input_filepath = argv[1];
    std::vector<std::byte> data = read_file(input_filepath);
    auto data_span = std::span{data};

    std::vector<std::byte> video_es;
    pg1::loop_ts_data(data_span, video_es);

    mpeg1::decode(video_es);
}
