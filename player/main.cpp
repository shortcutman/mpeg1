
#include <SDL3/SDL.h>
#include <print>

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

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }

        // --- RENDER FRAME HERE ---
        // Your Metalcpp rendering commands would go here.
        // --- RENDER FRAME HERE ---
    }

    // 5. Cleanup
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}