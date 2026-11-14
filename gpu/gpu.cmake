
add_library(
    gpu
    gpu/image.cpp
)
target_link_libraries(gpu PUBLIC
    libdanpg1
    metalcpp
    "-framework Foundation"
    "-framework Metal"
    "-framework QuartzCore"
)
target_include_directories(gpu PUBLIC gpu/)
