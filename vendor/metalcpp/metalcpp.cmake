
add_library(metalcpp INTERFACE)
target_include_directories(
    metalcpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
