cmake_minimum_required(VERSION 3.21)
project(Miro)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Lib)

option(MIRO_BUILD_TESTS "Build Miro internal tests" OFF)

if (PROJECT_IS_TOP_LEVEL OR MIRO_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
            NanoTest
            GIT_REPOSITORY https://github.com/eyalamirmusic/NanoTest.git
            GIT_TAG main
    )

    FetchContent_MakeAvailable(NanoTest)
    enable_testing()
    add_subdirectory(Tests)
    add_subdirectory(Benchmark)
endif ()

