set(CMAKE_FOLDER Examples)

# Generated files live in the build tree — they're build outputs, not
# committed source.
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Build the codegen tool for CalcApi (no implicit emit — we drive it with an
# explicit OUTPUT command so the generated files are proper build outputs).
miro_export(CalcSchema
        API_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cpp/CalcApi.h
        API CalcApi)

set(GEN_SWIFT
        ${GEN}/Schema.swift
        ${GEN}/Schema.runtime.swift
        ${GEN}/Schema.client.swift)

add_custom_command(
        OUTPUT ${GEN_SWIFT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN}
        COMMAND CalcSchema_Codegen --out ${GEN} --name Schema
                --format swift --format swift-runtime --format swift-client
        DEPENDS CalcSchema_Codegen
        COMMENT "Generating Swift types + runtime + client from CalcApi"
        VERBATIM)
add_custom_target(CalcGenerate DEPENDS ${GEN_SWIFT})

# The C++ host as a shared library: it bundles the Miro objects it needs
# and links the C++ runtime itself, so the Swift executable that links it
# only has to resolve the C-ABI symbols (calc_host_* / miro_string_free).
add_library(CalcHost SHARED cpp/Host.cpp)
target_link_libraries(CalcHost PRIVATE Miro)
target_include_directories(CalcHost PUBLIC cpp)
if (TARGET miro_warnings)
    target_link_libraries(CalcHost PRIVATE miro_warnings)
endif ()

# The Swift app — only when a usable Swift compiler is available, which on
# macOS means configuring with the Ninja generator (the Makefiles generator
# has no Swift support, so this skips cleanly there).
include(CheckLanguage)
check_language(Swift)

if (CMAKE_Swift_COMPILER)
    enable_language(Swift)

    set_source_files_properties(${GEN_SWIFT} PROPERTIES GENERATED TRUE)

    add_executable(CalcDemo swift/main.swift ${GEN_SWIFT})
    add_dependencies(CalcDemo CalcGenerate)
    target_link_libraries(CalcDemo PRIVATE CalcHost)

    # CalcHost.h is the Objective-C/C bridging header that surfaces the C
    # ABI to Swift. SHELL: keeps the flag and its path argument together.
    target_compile_options(CalcDemo PRIVATE
            "$<$<COMPILE_LANGUAGE:Swift>:SHELL:-import-objc-header ${CMAKE_CURRENT_SOURCE_DIR}/cpp/CalcHost.h>")

    add_test(NAME CalcDemo COMMAND CalcDemo)
else ()
    message(STATUS
            "MiroSwiftExport: no Swift compiler detected (on macOS, configure "
            "with -G Ninja). Skipping the CalcDemo Swift app; the C++ host "
            "library and codegen still build.")
endif ()
