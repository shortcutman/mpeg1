
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
#include <semaphore>
#include <thread>

namespace player {
    class Player {
    private:
        NS::SharedPtr<MTL::Device> _metal_device;
        NS::SharedPtr<MTL::Texture> _texture_current;
        NS::SharedPtr<MTL::Texture> _texture_next;
        std::binary_semaphore _queue{0};
        std::thread _video_decode_async;
        float _decode_frame_rate;
        bool _decode_video = true;
        
        std::vector<std::byte> _video_data;
        std::vector<std::byte> _audio_data;
        mpeg1::Decoder _video_decoder;
        mpeg1_aud::Decoder _audio_decoder;
        std::optional<SDL_TimerID> _timer;

        SDL_AudioSpec _audio_spec;
        SDL_AudioStream* _audio_stream;

    public:
        Player(NS::SharedPtr<MTL::Device> device);
        ~Player();

        bool open(std::string filepath);

        MTL::Texture* texture() const;
        float decode_frame_rate();

        bool isPlaying();
        void play();
        void stop();

    private:
        uint32_t play_advance();
        void step_frame_forward();

        void buffer_video();
        void buffer_audio();
    };
}
