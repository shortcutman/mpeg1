cmake_minimum_required(VERSION 4.0)

project(danpg1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_COMPILE_WARNING_AS_ERROR True)

add_compile_options(-Wall)

add_library(libdanpg1
    src/bitspan.cpp
    src/mpegts.cpp
    src/mpeg1.cpp
    src/mpeg1.consts.cpp
    src/idct.cpp
)
target_include_directories(libdanpg1 INTERFACE src/)

add_executable(danpg1
    src/main.cpp
)

target_link_libraries(
    danpg1
    libdanpg1
)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(danpg1_tests
    src/tests/bitspan.tests.cpp
    src/tests/mpegts.tests.cpp
    src/tests/mpeg1.tests.cpp
    src/tests/mpeg1.vlc.tests.cpp
)
target_link_libraries(
    danpg1_tests
    libdanpg1
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(danpg1_tests)