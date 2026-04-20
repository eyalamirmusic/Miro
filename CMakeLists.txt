cmake_minimum_required(VERSION 3.21)
project(Miro)

set(CMAKE_CXX_STANDARD 20)

if (PROJECT_IS_TOP_LEVEL)
    if (MSVC)
        add_compile_options(/W4)
    else ()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif ()
endif ()

add_subdirectory(Lib)

option(MIRO_BUILD_TESTS "Build Miro internal tests" OFF)

if (PROJECT_IS_TOP_LEVEL OR MIRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif ()
