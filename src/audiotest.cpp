
//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "mpegts.hpp"
#include "mpeg1_aud/audiodecoder.hpp"

// extern "C" {
//     #include "mpeg1_aud/tests/kjmp2.h"
// }

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

#define SAMPLE_RATE 44100
#define CHANNELS 2      // Stereo
#define AMPLITUDE 0.5f  // Volume (0.0 to 1.0)
#define FREQUENCY 440.0 // A4 Note

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("Args are wrong");
    }

    std::string input_filepath = argv[1];
    std::vector<std::byte> data = read_file(input_filepath);
    auto data_span = std::span{data};

    std::vector<std::byte> video_es, audio_es;
    pg1::loop_ts_data(data_span, video_es, audio_es);

    if (!SDL_Init(SDL_INIT_AUDIO)) {
        std::println("Could not initialise SDL: {}", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16LE;
    spec.channels = 2;
    spec.freq = 44100;

    auto stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

    if (!stream) {
        std::println("Failed to create aud stream: {}", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_ResumeAudioStreamDevice(stream);

    int running = 1;

    std::span<std::byte> span = audio_es;

    // kjmp2_context_t kctx;
    // kjmp2_init(&kctx);

    mpeg1_aud::Decoder audio_decoder;
    audio_decoder.set_data(span);
    mpeg1_aud::DecodedSamples samples;
    
    while (running) {
        // mpeg1_aud::align_to_sync(span);
        // std::array<signed short, 1152 * 2> frame;
        // auto bytes_read = kjmp2_decode_frame(&kctx, reinterpret_cast<unsigned char*>(span.data()), frame.data());
        // span = span.subspan(bytes_read);
        // if (bytes_read > 0)
        //     if (!SDL_PutAudioStreamData(stream, frame.data(), frame.size() * 2)) {
        //         SDL_Log("Failed to push data: %s", SDL_GetError());
        //         running = 0;
        //     }

        auto queued_bytes = SDL_GetAudioStreamQueued(stream);
        if (queued_bytes < (spec.freq * sizeof(short))) {
            if (audio_decoder.next_frame(samples) != 0 &&
                !SDL_PutAudioStreamData(stream, samples.data(), samples.size() * 2)) {
                SDL_Log("Failed to push data: %s", SDL_GetError());
                running = 0;
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = 0;
        }
    }

    SDL_DestroyAudioStream(stream);
    SDL_Quit();
    return 0;
}
