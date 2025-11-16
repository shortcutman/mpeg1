cmake_minimum_required(VERSION 4.0)

project(danpg1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_COMPILE_WARNING_AS_ERROR True)

add_compile_options(-Wall)

add_library(libdanpg1
    src/bitspan.cpp
    src/colour.cpp
    src/mpegts.cpp
    src/idct.cpp
    src/mpeg1_vid/constants.cpp
    src/mpeg1_vid/decode.cpp
    src/mpeg1_vid/mpeg1.cpp
    src/util.cpp
)
target_include_directories(libdanpg1 PUBLIC src/)

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
    src/mpeg1_vid/tests/decode.tests.cpp
    src/mpeg1_vid/tests/mpeg1.tests.cpp
    src/mpeg1_vid/tests/vlctables.tests.cpp
    src/tests/bitspan.tests.cpp
    src/tests/mpegts.tests.cpp
    src/tests/util.tests.cpp
)
target_link_libraries(
    danpg1_tests
    libdanpg1
    GTest::gtest_main
    GTest::gmock
)
include(GoogleTest)
gtest_discover_tests(danpg1_tests)