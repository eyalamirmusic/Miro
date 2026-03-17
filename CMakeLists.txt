cmake_minimum_required(VERSION 3.21)
project(Miro)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    NanoTest
    GIT_REPOSITORY https://github.com/eyalamirmusic/NanoTest.git
    GIT_TAG main
)

FetchContent_MakeAvailable(NanoTest)

enable_testing()

add_subdirectory(Lib)
add_subdirectory(Tests)
add_subdirectory(Benchmark)