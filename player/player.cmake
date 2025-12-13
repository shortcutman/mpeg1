
add_executable(
    danpg1_player
    player/main.cpp
)

target_link_libraries(danpg1_player PRIVATE SDL3::SDL3 imgui)
