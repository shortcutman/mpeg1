
add_executable(
    danpg1_player
    player/main.cpp
    player/player.cpp
)

target_link_libraries(danpg1_player PRIVATE
    SDL3::SDL3
    imgui
    metalcpp
    libdanpg1
    gpu
    "-framework Foundation"
    "-framework Metal"
    "-framework QuartzCore"
)
