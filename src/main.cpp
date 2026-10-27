
//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <chrono>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <iostream>

#include "colour.hpp"
#include "mpegts.hpp"
#include "mpeg1_vid/decoder.hpp"

#include <CLI/CLI.hpp>

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

void dump_file(const std::vector<std::byte>& data, const std::string& filename) {
    std::ofstream file;
    file.open(filename, file.binary | file.out);

    if (!file.is_open()) {
        throw std::runtime_error("Couldn't open for writing.");
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
}

}

int main(int argc, char** argv) {
    std::string input_filepath;
    bool dumpFrames = false, dumpVideo = false;
    bool printFrameCount = false;
    uint64_t startFrame = 0, endFrame = std::numeric_limits<uint64_t>::max();

    CLI::App app{"danpg1 MPEG1-Video inside MPEG2-TS decoder"};
    app.add_option("file", input_filepath, "Input video file.")->required();
    app.add_option("--start-frame", startFrame, "Start decoding from input video at frame number.");
    app.add_option("--end-frame", endFrame, "Finish decoding from input video at frame number.");
    app.add_flag("-F,--frame-count", printFrameCount, "Print incrementing frame count during decode.");
    app.add_flag("-D,--dump-frames", dumpFrames, "Dump decoded frames as PPM image files.");
    app.add_flag("--dump-video", dumpVideo, "Dump video elementary stream.");
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    std::vector<std::byte> data = read_file(input_filepath);
    auto data_span = std::span{data};

    std::vector<std::byte> video_es, audio_es;
    pg1::loop_ts_data(data_span, video_es, audio_es);

    if (dumpVideo) {
        dump_file(video_es, "/tmp/danpg1/video.mpg");
    }

    mpeg1::Decoder decoder;
    decoder.set_data(video_es);

    // std::string input;
    // std::print("Input to start decoding...");
    // std::getline(std::cin, input);

    auto start = std::chrono::steady_clock::now();
    size_t frame_number = 1;
    for (auto frame = decoder.next_frame(frame_number >= startFrame); frame.has_value() && frame_number < endFrame; frame = decoder.next_frame(frame_number >= startFrame)) {

        if (printFrameCount) {
            std::println("Frame: {} {}", frame_number, frame->description);
        }

        if (dumpFrames) {
            image::writeOutPPM(
                std::format("/tmp/danpg1/frame_{:04d}_{}.ppm", frame_number, frame->description),
                *frame);
        } else {
            auto r = frame->as_float_rgba_vector();
            (void)r;
        }

        frame_number++;
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::println("Total of {} frames.", frame_number);
    std::println("Decode took {} ms.", duration.count());
    std::println("Average of {} ms per frame.", duration.count() / frame_number);
}
