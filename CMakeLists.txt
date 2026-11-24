cmake_minimum_required(VERSION 3.20)
project(SplitSecondVelocityReconstruction VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
)

# Core & Asset Engine Library
add_library(ssv_core STATIC
    src/platform/platform_null.cpp
    src/assets/lzss_decomp.cpp
    src/assets/ark_parser.cpp
    src/core/state_machine.cpp
    src/core/game_engine.cpp
)

# Test Suite Executable
add_executable(ssv_tests
    tests/test_runner.cpp
    tests/test_math.cpp
    tests/test_state_machine.cpp
    tests/test_ark_parser.cpp
)

target_link_libraries(ssv_tests PRIVATE ssv_core)
