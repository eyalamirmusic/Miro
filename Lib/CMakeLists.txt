find_package(EADataStructures REQUIRED)
find_package(ResEmbed REQUIRED)

set(CMAKE_FOLDER Miro)

add_library(Miro STATIC
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

# Pre-built main() for executables created via miro_export().
# OBJECT (rather than STATIC) so the helper can plug it directly into
# add_executable() as a source — sidesteps the empty-target check and
# makes main() visible to the linker without needing WHOLE_ARCHIVE.
#
# This target — and the executables built from it — only run as a
# build-time step on the developer's machine, never ship to users, so
# we skip it when cross-compiling (the post-build step can't run a
# foreign-arch executable anyway). Filesystem access goes through
# ghc::filesystem so we don't need a macOS 10.15+ deployment target.
if (NOT CMAKE_CROSSCOMPILING)
    add_library(MiroTypeExportMain OBJECT Miro/TypeExport/Main.cpp)
    target_link_libraries(MiroTypeExportMain
        PUBLIC Miro
        PRIVATE ghcFilesystem::ghc_filesystem)
endif ()
