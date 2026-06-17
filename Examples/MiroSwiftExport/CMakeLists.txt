set(CMAKE_FOLDER Examples)

# Generate Schema.swift + Schema.client.swift from CalcApi using the real
# codegen machinery (the registered `swift` / `swift-client` formats).
# miro_export builds CalcSchema_Codegen and attaches a POST_BUILD step
# that re-emits the files into generated/ whenever the schema changes.
miro_export(CalcSchema
        API_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cpp/CalcApi.h
        API CalcApi
        OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated
        OUTPUT_NAME Schema
        FORMATS swift swift-client)

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
# macOS means configuring with the Ninja generator (the Makefiles
# generator has no Swift support, so this skips cleanly there).
include(CheckLanguage)
check_language(Swift)

if (CMAKE_Swift_COMPILER)
    enable_language(Swift)

    add_executable(CalcDemo
            swift/main.swift
            generated/Schema.swift
            generated/Schema.client.swift)

    # Ensure the generated sources are refreshed before Swift compiles.
    add_dependencies(CalcDemo CalcSchema_Codegen)

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
