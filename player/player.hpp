
//------------------------------------------------------------------------------
// player.hpp
//------------------------------------------------------------------------------

#pragma once

#include "mpeg1_vid/decoder.hpp"
#include "mpeg1_aud/audiodecoder.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <optional>
#include <queue>
#include <semaphore>
#include <thread>

namespace player {
    class Player {
    private:
        const size_t FRAME_BUFFER_COUNT = 5;

        NS::SharedPtr<MTL::Device> _metal_device;
        NS::SharedPtr<MTL::Texture> _texture_current;

        std::queue<NS::SharedPtr<MTL::Texture>> _frame_queue;
        std::vector<NS::SharedPtr<MTL::Texture>> _reuse_queue;

        std::binary_semaphore _fill_queue{0};
        std::binary_semaphore _edit_queue{0};

        std::thread _video_decode_async;
        float _decode_frame_rate;
        bool _decode_video = true;
        bool _play = false;
        
        std::vector<std::byte> _video_data;
        std::vector<std::byte> _audio_data;
        mpeg1::Decoder _video_decoder;
        mpeg1_aud::Decoder _audio_decoder;
        uint64_t _start_ns = 0;
        uint64_t _frames = 0;
        uint64_t _last_frame_time_ms = 0;

        SDL_AudioSpec _audio_spec;
        SDL_AudioStream* _audio_stream;

    public:
        Player(NS::SharedPtr<MTL::Device> device);
        ~Player();

        bool open(std::string filepath);

        MTL::Texture* texture() const;
        void tick();
        float decode_frame_rate();

        bool isPlaying();
        void play();
        void stop();

    private:
        bool play_advance();

        void buffer_video();
        void buffer_audio();
    };
}
