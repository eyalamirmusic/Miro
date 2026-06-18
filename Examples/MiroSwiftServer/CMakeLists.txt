set(CMAKE_FOLDER Examples)

# Generated files live in the build tree — they're build outputs, not
# committed source.
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Build the codegen tool for CalcApi (no implicit emit — we drive it with an
# explicit OUTPUT command below so the generated files are proper build
# outputs and don't need to exist at configure time).
miro_export(CalcServerSchema
        API_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cpp/CalcApi.h
        API CalcApi)

set(GEN_SWIFT
        ${GEN}/Schema.swift
        ${GEN}/Schema.runtime.swift
        ${GEN}/Schema.server.swift)
set(GEN_CPP
        ${GEN}/Schema.miro.h
        ${GEN}/Schema.client.h)

add_custom_command(
        OUTPUT ${GEN_SWIFT} ${GEN_CPP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN}
        COMMAND CalcServerSchema_Codegen --out ${GEN} --name Schema
                --format cpp-miro --format cpp-client
                --format swift --format swift-runtime --format swift-server
        DEPENDS CalcServerSchema_Codegen
        COMMENT "Generating C++ client + Swift server from CalcApi"
        VERBATIM)
add_custom_target(CalcServerGenerate DEPENDS ${GEN_SWIFT} ${GEN_CPP})

# The Swift app needs a Swift compiler — on macOS that means the Ninja
# generator. Skip cleanly otherwise (the codegen target still works).
include(CheckLanguage)
check_language(Swift)

if (CMAKE_Swift_COMPILER)
    enable_language(Swift)

    set_source_files_properties(${GEN_SWIFT} PROPERTIES GENERATED TRUE)

    # Swift dynamic library: generated types + runtime + server (with the
    # @_cdecl adapter) + the app's Handlers. Self-contains the Swift runtime
    # and exports the C-ABI symbols the C++ side calls. Shared so the C++
    # executable only has to resolve those C symbols.
    add_library(CalcSwift SHARED ${GEN_SWIFT} swift/Handlers.swift)
    add_dependencies(CalcSwift CalcServerGenerate)

    # C++ driver: builds the generated client over makeRemoteInvoker and
    # asserts. Links libMiro + the Swift dylib.
    add_executable(CalcServerDemo cpp/main.cpp)
    add_dependencies(CalcServerDemo CalcServerGenerate)
    target_include_directories(CalcServerDemo PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/cpp
            ${GEN})
    target_link_libraries(CalcServerDemo PRIVATE Miro CalcSwift)
    if (TARGET miro_warnings)
        target_link_libraries(CalcServerDemo PRIVATE miro_warnings)
    endif ()

    add_test(NAME CalcServerDemo COMMAND CalcServerDemo)
else ()
    message(STATUS
            "MiroSwiftServer: no Swift compiler detected (on macOS, configure "
            "with -G Ninja). Skipping CalcServerDemo; the codegen still runs.")
endif ()
