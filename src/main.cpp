
//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <iostream>

#include "colour.hpp"
#include "mpegts.hpp"
#include "mpeg1_vid/decoder.hpp"

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

// void dump_file(const std::vector<std::byte>& data, const std::string& filename) {
//     std::ofstream file;
//     file.open(filename, file.binary | file.out);

//     if (!file.is_open()) {
//         throw std::runtime_error("Couldn't open for writing.");
//     }

//     file.write(reinterpret_cast<const char*>(data.data()), data.size());
//     file.close();
// }

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

    // dump_file(video_es, "/tmp/danpg1/video.mpg");

    mpeg1::Decoder decoder;
    decoder.set_data(video_es);

    size_t frame_number = 0;
    for (auto frame = decoder.next_frame(); frame.has_value(); frame = decoder.next_frame()) {
        frame_number++;
        std::println("Frame: {} {}", frame_number, frame->description);
        image::writeOutPPM(
            std::format("/tmp/danpg1/frame_{:04d}_{}.ppm", frame_number, frame->description),
            frame->encoded_width,
            frame->encoded_height,
            frame->image);
    }
}
