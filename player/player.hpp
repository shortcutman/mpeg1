
//------------------------------------------------------------------------------
// player.hpp
//------------------------------------------------------------------------------

#pragma once

#include "mpeg1_vid/decoder.hpp"

#include <SDL3/SDL_timer.h>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <optional>

namespace player {
    class Player {
    private:
        NS::SharedPtr<MTL::Texture> _texture;
        std::vector<std::byte> _data;
        mpeg1::Decoder _decoder;
        std::optional<SDL_TimerID> _timer;

    public:
        Player(NS::SharedPtr<MTL::Texture> texture);
        ~Player();

        bool open(std::string filepath);

        bool isPlaying();
        void play();
        void stop();

    private:
        void step_frame_forward();
    };
}
