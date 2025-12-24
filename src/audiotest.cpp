
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

    while (running) {
        std::span<std::byte> span = audio_es;
        mpeg1_aud::align_to_sync(span);

        auto header = mpeg1_aud::read_frame_header(span);
        util::bitspan bits(span);
        auto allocations = mpeg1_aud::read_allocations(bits, header);
        auto scfsi = mpeg1_aud::read_scfsi(bits, allocations);
        auto scale_factors = mpeg1_aud::read_scale_factors(bits, allocations, scfsi);
        auto decoded = mpeg1_aud::decode_samples(bits, allocations, scale_factors);

        // --- Push Data ---
        // Pass the size in BYTES (sizeof handles this automatically)
        if (!SDL_PutAudioStreamData(stream, decoded.data(), decoded.size() * 2)) {
            SDL_Log("Failed to push data: %s", SDL_GetError());
            running = 0;
        }

        // // --- Throttle ---
        // int queued_bytes = SDL_GetAudioStreamQueued(stream);
        // int bytes_per_sec = SAMPLE_RATE * CHANNELS * sizeof(int16_t);
        
        // // If we have more than 0.5s buffered, wait
        // if (queued_bytes > (bytes_per_sec * 0.5)) {
        //     SDL_Delay(10); 
        // }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = 0;
        }
    }

    SDL_DestroyAudioStream(stream);
    SDL_Quit();
    return 0;
}
