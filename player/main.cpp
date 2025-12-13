
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

int main(int argc, char** argv) {
    std::println("Hello world!");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("Could not initialise SDL: {}", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "SDL3 Metalcpp Demo",
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

        bool demowindow = false;
        ImGui::ShowDemoWindow(&demowindow);

        {
            ImGui::Begin("Player");

            ImGui::Text("This is where the player will go.");

            ImGui::End();
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