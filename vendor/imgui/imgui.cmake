
include(FetchContent)
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.5.zip
)
FetchContent_MakeAvailable(imgui)

add_library(
    imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm"
)
target_include_directories(
    imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)
target_link_libraries(
    imgui PUBLIC
    SDL3::SDL3
    metalcpp
    "-framework Foundation"
    "-framework Metal"
    "-framework QuartzCore"
)

set_source_files_properties(
    "${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm"
    PROPERTIES
    COMPILE_OPTIONS "-ObjC++;-fobjc-weak;-fobjc-arc"
)

target_compile_definitions(imgui PRIVATE IMGUI_IMPL_METAL_CPP)