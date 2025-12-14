
#include "mpegts.hpp"
#include "mpeg1_vid/decoder.hpp"

#include <SDL3/SDL.h>
#include <print>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#define IMGUI_IMPL_METAL_CPP
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_metal.h>

#include <span>
#include <string>
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

void frame_to_texture(const mpeg1::Decoder::Frame& frame, MTL::Texture* texture) {
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

int main(int argc, char** argv) {
    std::println("Hello world!");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("Could not initialise SDL: {}", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "danpg1 player",
        800, 600,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_METAL
    );

    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    auto metalDevice = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    SDL_MetalView view = SDL_Metal_CreateView(window);
    auto layer = reinterpret_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(view));
    layer->setDevice(metalDevice.get());
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    auto commandQueue = NS::TransferPtr(layer->device()->newCommandQueue());
    auto renderPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(layer->device());
    ImGui_ImplSDL3_InitForMetal(window);

    auto textureDescriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA32Float);
    textureDescriptor->setWidth(640);
    textureDescriptor->setHeight(272);
    auto texture = NS::TransferPtr(metalDevice->newTexture(textureDescriptor.get()));

    std::string input_filepath = argv[1];
    std::vector<std::byte> data = read_file(input_filepath);
    auto data_span = std::span{data};

    std::vector<std::byte> video_es;
    pg1::loop_ts_data(data_span, video_es);
    mpeg1::Decoder decoder;
    decoder.set_data(video_es);
    auto frameagain = decoder.next_frame();
    if (frameagain.has_value()) {
        frame_to_texture(frameagain.value(), texture.get());
    }

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }

        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        layer->setDrawableSize(CGSizeMake(width, height));
        auto drawable = layer->nextDrawable();

        auto commandBuffer = commandQueue->commandBuffer();
        float clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        renderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]));
        renderPassDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
        renderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        renderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        auto renderEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor.get());
        renderEncoder->pushDebugGroup(NS::String::string("imgui demo", NS::StringEncoding::ASCIIStringEncoding));

        ImGui_ImplMetal_NewFrame(renderPassDescriptor.get());
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::Begin("Player", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            ImGui::Text("%s", std::format("Playing file: {}", input_filepath).c_str());

            auto aspect_ratio = 272.f / 640.f;
            auto space = ImGui::GetContentRegionAvail();
            auto vert = space.x * aspect_ratio;
            ImGui::Image((ImTextureID)(intptr_t)(texture.get()), ImVec2(space.x, vert));

            if (ImGui::Button("Next Frame")) {
                auto frame_res = decoder.next_frame();
                if (frame_res.has_value()) {
                    frame_to_texture(frame_res.value(), texture.get());
                }
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplMetal_RenderDrawData(draw_data, commandBuffer, renderEncoder);

        renderEncoder->popDebugGroup();
        renderEncoder->endEncoding();

        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();
    }

    // 5. Cleanup
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}