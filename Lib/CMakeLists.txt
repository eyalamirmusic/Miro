add_library(Miro STATIC
    Miro/Cpp/Cpp.cpp
    Miro/JSON/Json.cpp
    Miro/JSON/Parser.cpp
    Miro/JSON/Printer.cpp
    Miro/Reflection/Any.cpp
    Miro/Reflection/CommandTable.cpp
    Miro/Reflection/JsonReflector.cpp
    Miro/Reflection/Reflector.cpp
    Miro/Schema/Schema.cpp
    Miro/TypeTree/TypeTree.cpp
    Miro/TypeScript/TypeScript.cpp)
target_include_directories(Miro PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(Miro PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

if (MIRO_UNITY_BUILD)
    set_target_properties(Miro PROPERTIES UNITY_BUILD ON)
endif ()

# Pre-built main() for executables created via miro_add_type_export().
# OBJECT (rather than STATIC) so the helper can plug it directly into
# add_executable() as a source — sidesteps the empty-target check and
# makes main() visible to the linker without needing WHOLE_ARCHIVE.
#
# This target — and the executables built from it — only run as a
# build-time step on the developer's machine, never ship to users. So we
# skip it when cross-compiling (the post-build step can't run a
# foreign-arch executable anyway), and on Apple we override the
# deployment target locally so std::filesystem (macOS 10.15+) is
# available even when the surrounding project pins something older.
if (NOT CMAKE_CROSSCOMPILING)
    add_library(MiroTypeExportMain OBJECT Miro/TypeExport/Main.cpp)
    target_link_libraries(MiroTypeExportMain PUBLIC Miro)

    if (APPLE)
        target_compile_options(MiroTypeExportMain PRIVATE
            -mmacosx-version-min=10.15)
    endif ()
endif ()
