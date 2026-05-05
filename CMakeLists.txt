cmake_minimum_required(VERSION 3.29)
project(Miro)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(Warnings)
include(MiroTypeExport)

option(MIRO_UNITY_BUILD "Enable CMake unity build for the Miro library" ON)
option(MIRO_BUILD_TESTS "Build Miro internal tests" OFF)

add_subdirectory(SDKs)
add_subdirectory(Lib)

if (PROJECT_IS_TOP_LEVEL OR MIRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
    add_subdirectory(Examples)
endif ()
