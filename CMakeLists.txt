cmake_minimum_required(VERSION 3.31)
project(Miro)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MIRO_HIDDEN_VISIBILITY
        "Compile Miro with hidden symbol visibility" ON)

if (MIRO_HIDDEN_VISIBILITY)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(Warnings)
include(MiroTypeExport)

option(MIRO_UNITY_BUILD "Enable CMake unity build for the Miro library" OFF)
option(MIRO_BUILD_TESTS "Build Miro internal tests" OFF)
option(MIRO_BUILD_EXAMPLES "Build Miro examples" PROJECT_IS_TOP_LEVEL)

add_subdirectory(SDKs)
add_subdirectory(Lib)

if (PROJECT_IS_TOP_LEVEL OR MIRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif ()

if (MIRO_BUILD_EXAMPLES)
    enable_testing()
    add_subdirectory(Examples)
endif ()
