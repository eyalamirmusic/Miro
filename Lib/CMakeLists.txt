find_package(EADataStructures REQUIRED)
find_package(ResEmbed REQUIRED)

set(CMAKE_FOLDER Miro)

add_library(Miro STATIC
    Miro/Bridge/BindReflector.cpp
    Miro/Bridge/Bridge.cpp
    Miro/CommandExport/CommandExport.cpp
    Miro/CommandExport/Register.cpp
    Miro/CommandExport/ResolvedTypes.cpp
    Miro/Cpp/Cpp.cpp
    Miro/Cpp/CppClient.cpp
    Miro/Detail/StringUtilities.cpp
    Miro/JSON/Json.cpp
    Miro/JSON/Parser.cpp
    Miro/JSON/Printer.cpp
    Miro/Reflection/Any.cpp
    Miro/Reflection/CommandTable.cpp
    Miro/Reflection/JsonReflector.cpp
    Miro/Reflection/Reflector.cpp
    Miro/Schema/Schema.cpp
    Miro/TypeExport/CodegenMain.cpp
    Miro/TypeExport/Format.cpp
    Miro/TypeExport/Register.cpp
    Miro/TypeTree/TypeTree.cpp
    Miro/TypeScript/TypeScript.cpp)
target_include_directories(Miro PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(Miro PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)
target_link_libraries(Miro
    PUBLIC ea_data_structures
    PRIVATE miro_warnings)

res_embed_add(Miro
    FILES Miro/TypeScript/Resources/BridgeRuntime.ts
    NAMESPACE MiroResources
    CATEGORY MiroResources)

if (MIRO_UNITY_BUILD)
    set_target_properties(Miro PROPERTIES UNITY_BUILD ON)
endif ()

# Built-in format registrations (zod / ts / backend / ts-server /
# bridge / jsonschema / cpp / cpp-miro / cpp-client). Split out as its
# own OBJECT library so it can be linked into both the codegen
# executables AND the test runner. Plain static-library placement
# would let the linker drop the registrations (nothing references
# their symbols — the work happens at static init); OBJECT placement
# splices the .o files directly into the consumer's link line so the
# initializers always survive.
if (NOT CMAKE_CROSSCOMPILING)
    add_library(MiroFormats OBJECT
        Miro/TypeExport/BuiltinFormats.cpp)
    target_link_libraries(MiroFormats PUBLIC Miro)
endif ()

# Pre-built main() for executables created via miro_export(). Includes
# CodegenWriter.cpp (file I/O via ghc::filesystem). Build-time only —
# skipped when cross-compiling, since the post-build step can't run a
# foreign-arch executable anyway.
if (NOT CMAKE_CROSSCOMPILING)
    add_library(MiroTypeExportMain OBJECT
        Miro/TypeExport/CodegenWriter.cpp
        Miro/TypeExport/Main.cpp)
    target_link_libraries(MiroTypeExportMain
        PUBLIC Miro MiroFormats
        PRIVATE ghcFilesystem::ghc_filesystem)
endif ()
