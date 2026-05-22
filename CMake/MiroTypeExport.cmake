# miro_export — declare a schema that's compiled, then walked at build
# time to emit generated artifacts.
#
# Produces one user-facing target plus one helper:
#
#   ${NAME}              INTERFACE library carrying the generated header
#                        include dirs. Linking it gives a consumer the
#                        typed wrappers. The registration sources sit on
#                        a target property and are spliced into consumers
#                        on demand by eacp_target_uses_schema(... HANDLERS).
#
#   ${NAME}_Codegen      Build-time executable that links the registration
#                        sources and walks the registry. Each
#                        miro_export_emit() call appends a POST_BUILD step
#                        that re-runs the exec to emit one set of files.
#                        Consumers add_dependencies() against it (via
#                        eacp_target_uses_schema) so the exec — and its
#                        POST_BUILD emits — fire before the consumer
#                        compiles.
#
# Single-call shortcut: pass OUTPUT_DIR + FORMATS to miro_export() and an
# emit step is added implicitly. Call miro_export_emit() afterwards for
# additional emits (e.g. cpp headers into a different directory).
#
# Two modes — exactly one of SOURCES / API must be supplied:
#
#   miro_export(MySchema
#       SOURCES Registrations.cpp       # static-init mode (legacy)
#       [OUTPUT_DIR ...] [OUTPUT_NAME ...] [FORMATS ...])
#
#   miro_export(MySchema
#       API_HEADER Api/TodoApi.h        # API mode (inversion path)
#       API Api::Todos [Api::Other]     # fully-qualified API class(es)
#       [OUTPUT_DIR ...] [OUTPUT_NAME ...] [FORMATS ...])
#
# In SOURCES mode the user's TUs carry MIRO_EXPORT_COMMAND(S) macros;
# the codegen executable links them + MiroTypeExportMain's pre-built
# main(), which walks the static-init registries.
#
# In API mode no static-init macros are needed. The user declares one
# or more API classes with a static or member `void reflect(Miro::
# ApiReflector&)` method. miro_export generates a small stub main:
#
#   #include <Miro/Miro.h>
#   #include "<API_HEADER>"            // each header passed via API_HEADER
#   int main(int argc, char** argv)
#   {
#       return Miro::TypeExport::codegenMain<Api::Todos[, ...]>(argc, argv);
#   }
#
# The resulting executable links MiroFormats (format registrations) +
# MiroCodegenWriter (file I/O), not MiroTypeExportMain, so it doesn't
# pull in the static-init main() that would conflict with the stub.
function(miro_export NAME)
    set(oneValueArgs OUTPUT_DIR OUTPUT_NAME)
    set(multiValueArgs FORMATS SOURCES API API_HEADER)
    cmake_parse_arguments(ME "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (ME_SOURCES AND ME_API)
        message(FATAL_ERROR
                "miro_export(${NAME}): SOURCES and API are mutually exclusive")
    endif ()
    if (NOT ME_SOURCES AND NOT ME_API)
        message(FATAL_ERROR
                "miro_export(${NAME}): one of SOURCES or API is required")
    endif ()
    if (ME_API AND NOT ME_API_HEADER)
        message(FATAL_ERROR
                "miro_export(${NAME}): API_HEADER is required when API is set")
    endif ()

    add_library(${NAME} INTERFACE)

    if (ME_SOURCES)
        # Resolve sources to absolute paths up front. They get spliced
        # into consumers in other directories via eacp_target_uses_schema,
        # where relative paths would resolve against the wrong
        # CMAKE_CURRENT_SOURCE_DIR.
        set(absSources "")
        foreach (src IN LISTS ME_SOURCES)
            if (IS_ABSOLUTE "${src}")
                list(APPEND absSources "${src}")
            else ()
                list(APPEND absSources "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
            endif ()
        endforeach ()
        set_property(TARGET ${NAME} PROPERTY MIRO_SCHEMA_SOURCES ${absSources})
    endif ()

    # The codegen executable runs as a build-time tool on the host. When
    # cross-compiling there's no way to run a foreign-arch executable on
    # the build machine, so we skip creating it — caller-side
    # add_dependencies(consumer ${NAME}_Codegen) becomes a no-op and
    # the project links against committed generated files.
    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    if (ME_SOURCES)
        add_executable(${NAME}_Codegen
                ${absSources} $<TARGET_OBJECTS:MiroTypeExportMain>)
        target_link_libraries(${NAME}_Codegen PRIVATE MiroTypeExportMain)
    else ()
        # Resolve API_HEADER paths so the generated stub's #include lines
        # work from the build directory.
        set(absHeaders "")
        foreach (hdr IN LISTS ME_API_HEADER)
            if (IS_ABSOLUTE "${hdr}")
                list(APPEND absHeaders "${hdr}")
            else ()
                list(APPEND absHeaders "${CMAKE_CURRENT_SOURCE_DIR}/${hdr}")
            endif ()
        endforeach ()

        # Generate stub_main.cpp: includes each API header, then dispatches
        # to codegenMain<...> with the user's API class list. file(WRITE)
        # is configure-time; the stub depends only on configure inputs so
        # this is correct (a rebuild is triggered if any of those change).
        #
        # Semicolons are escaped (\;) because CMake treats unescaped `;`
        # inside strings as list separators — they'd get eaten otherwise
        # and the emitted C++ wouldn't compile.
        set(stub "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_codegen_main.cpp")
        set(stubBody
                "// Generated by miro_export(${NAME} API ...). Do not edit.\n")
        set(stubBody "${stubBody}#include <Miro/Miro.h>\n")
        foreach (hdr IN LISTS absHeaders)
            set(stubBody "${stubBody}#include \"${hdr}\"\n")
        endforeach ()
        list(JOIN ME_API ", " apiTypeList)
        set(stubBody "${stubBody}\nint main(int argc, char** argv)\n")
        set(stubBody "${stubBody}{\n")
        set(stubBody
                "${stubBody}    return Miro::TypeExport::codegenMain<${apiTypeList}>(argc, argv)\;\n")
        set(stubBody "${stubBody}}\n")
        file(WRITE ${stub} ${stubBody})

        add_executable(${NAME}_Codegen
                ${stub}
                $<TARGET_OBJECTS:MiroFormats>
                $<TARGET_OBJECTS:MiroCodegenWriter>)
        target_link_libraries(${NAME}_Codegen
                PRIVATE Miro MiroFormats MiroCodegenWriter)
    endif ()

    if (TARGET miro_warnings)
        target_link_libraries(${NAME}_Codegen PRIVATE miro_warnings)
    endif ()

    if (ME_OUTPUT_DIR)
        miro_export_emit(${NAME}
                OUTPUT_DIR ${ME_OUTPUT_DIR}
                OUTPUT_NAME ${ME_OUTPUT_NAME}
                FORMATS ${ME_FORMATS})
    endif ()
endfunction()


# miro_export_emit — register a generation step on a schema declared via
# miro_export(). Each emit attaches a POST_BUILD command to the schema's
# codegen executable, so every emit reruns whenever the exec is rebuilt
# (i.e. whenever any registration source changes).
#
# Usage:
#   miro_export_emit(MySchema
#       OUTPUT_DIR  <dir>
#       OUTPUT_NAME <stem>           # optional; defaults to schema target name
#       FORMATS     ts backend ...)  # optional; defaults to all
function(miro_export_emit NAME)
    cmake_parse_arguments(MEE "" "OUTPUT_DIR;OUTPUT_NAME" "FORMATS" ${ARGN})

    if (NOT MEE_OUTPUT_DIR)
        message(FATAL_ERROR "miro_export_emit(${NAME}): OUTPUT_DIR is required")
    endif ()
    if (NOT TARGET ${NAME})
        message(FATAL_ERROR
                "miro_export_emit(${NAME}): target does not exist; "
                "call miro_export(${NAME} ...) first")
    endif ()
    if (NOT MEE_OUTPUT_NAME)
        set(MEE_OUTPUT_NAME ${NAME})
    endif ()

    target_include_directories(${NAME} INTERFACE ${MEE_OUTPUT_DIR})
    file(MAKE_DIRECTORY "${MEE_OUTPUT_DIR}")

    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    set(formatArgs "")
    foreach (fmt IN LISTS MEE_FORMATS)
        list(APPEND formatArgs --format ${fmt})
    endforeach ()

    add_custom_command(TARGET ${NAME}_Codegen POST_BUILD
            COMMAND ${NAME}_Codegen
                    --out ${MEE_OUTPUT_DIR}
                    --name ${MEE_OUTPUT_NAME}
                    ${formatArgs}
            COMMENT "Exporting types: ${NAME} -> ${MEE_OUTPUT_DIR}"
            VERBATIM)
endfunction()
