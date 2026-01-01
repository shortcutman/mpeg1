
//------------------------------------------------------------------------------
// player.cpp
//------------------------------------------------------------------------------

#include "player.hpp"

#include "mpegts.hpp"

#include <fstream>

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

void frame_to_texture(const image::Frame& frame, MTL::Texture* texture) {
    std::vector<std::array<float, 4>> img_float;

    for (auto c : frame.image) {
        float r = c.r / 255.f;
        float g = c.g / 255.f;
        float b = c.b / 255.f;
        img_float.push_back({r, g, b, 1.f});
    }

    MTL::Region region = {0, 0, 0, frame.encoded_width, frame.encoded_height, 1};
    texture->replaceRegion(region, 0, img_float.data(), 16 * frame.encoded_width);
}

}

player::Player::Player(NS::SharedPtr<MTL::Texture> texture)
: _texture(texture)
{
}

player::Player::~Player() {
    if (_timer) {
        SDL_RemoveTimer(*_timer);
    }
}

bool player::Player::open(std::string filepath) {
    std::vector<std::byte> data = read_file(filepath);
    if (data.empty()) {
        return false;
    }

    auto data_span = std::span{data};

    pg1::loop_ts_data(data_span, _video_data, _audio_data);
    _video_decoder.set_data(_video_data);
    _audio_decoder.set_data(_audio_data);
    auto first_frame = _video_decoder.next_frame();
    if (first_frame.has_value()) {
        frame_to_texture(first_frame.value(), _texture.get());
    } else {
        return false;
    }

    _audio_spec.format = SDL_AUDIO_S16LE;
    _audio_spec.channels = 2;
    _audio_spec.freq = 44100;
    
    _audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &_audio_spec, NULL, NULL);
    if (!_audio_stream) {
        std::println("Failed to create audio stream: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool player::Player::isPlaying() {
    return _timer.has_value();
}

void player::Player::play() {
    SDL_ResumeAudioStreamDevice(_audio_stream);
    _timer = SDL_AddTimer(0,
        static_cast<uint32_t(*)(void*, SDL_TimerID, uint32_t)>([](void* ctx, SDL_TimerID timerID, uint32_t interval) -> uint32_t {
            return reinterpret_cast<Player*>(ctx)->play_advance();
        }), this);
}

void player::Player::stop() {
    if (_timer) {
        SDL_RemoveTimer(*_timer);
        _timer.reset();
    }

    SDL_PauseAudioStreamDevice(_audio_stream);
}

uint32_t player::Player::play_advance() {
    step_frame_forward();
    buffer_audio();
    return 1000.0 / _video_decoder.frame_rate();
}

void player::Player::step_frame_forward() {
    auto frame = _video_decoder.next_frame();
    if (frame.has_value()) {
        frame_to_texture(frame.value(), _texture.get());
    }
}

void player::Player::buffer_audio() {
    mpeg1_aud::DecodedSamples samples;
    while (SDL_GetAudioStreamQueued(_audio_stream) < (_audio_spec.freq * sizeof(short))) {
        if (_audio_decoder.next_frame(samples) != 0 &&
            !SDL_PutAudioStreamData(_audio_stream, samples.data(), samples.size() * 2)) {
            std::println("Failed to put audio data: {}", SDL_GetError());
        }
    }
}