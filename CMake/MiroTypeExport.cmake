# miro_add_type_export — defines a build-time type-export executable plus
# a custom command that runs it. The executable's main() comes from the
# MiroTypeExportMain target; the user provides registrations via
# MIRO_EXPORT_TYPES(...) calls in libraries listed under LIBRARIES.
#
# Each non-INTERFACE library is linked with WHOLE_ARCHIVE so its static
# initializers (the registrations) survive linking. INTERFACE libraries
# are linked plainly — their transitive sources end up compiled directly
# into the exporter executable, so there's nothing to whole-archive.
#
# Usage:
#   miro_add_type_export(
#       NAME        MySchema
#       SOURCES     Registrations.cpp     # paths relative to caller's dir
#       LIBRARIES   MyExtraTypes          # optional
#       OUTPUT_DIR  ${CMAKE_CURRENT_BINARY_DIR}/ts
#       OUTPUT_NAME schema                # optional; defaults to NAME
#       FORMATS     zod ts                # optional; defaults to all
#   )
#
# At least one of SOURCES or LIBRARIES must be provided.
#
# Side effects:
#   - Creates a single executable target ${NAME}. A POST_BUILD step runs
#     it after each relink, regenerating the output files.
#   - One bundled file per format is written to OUTPUT_DIR, named
#     ${OUTPUT_NAME}.<extension> (e.g. schema.zod.ts, schema.ts).
function(miro_add_type_export)
    # The exporter is a build-time tool that runs as a POST_BUILD step on
    # the host. When cross-compiling there's no way to run a foreign-arch
    # executable on the build machine, so skip target creation entirely
    # and let the same CMakeLists.txt work for both host and cross builds.
    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    set(options "")
    set(oneValueArgs NAME OUTPUT_DIR OUTPUT_NAME)
    set(multiValueArgs FORMATS LIBRARIES SOURCES)
    cmake_parse_arguments(MTE
        "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT MTE_NAME)
        message(FATAL_ERROR "miro_add_type_export: NAME is required")
    endif ()
    if (NOT MTE_OUTPUT_DIR)
        message(FATAL_ERROR "miro_add_type_export: OUTPUT_DIR is required")
    endif ()
    if (NOT MTE_SOURCES AND NOT MTE_LIBRARIES)
        message(FATAL_ERROR
            "miro_add_type_export: at least one of SOURCES or LIBRARIES is required")
    endif ()
    if (NOT MTE_OUTPUT_NAME)
        set(MTE_OUTPUT_NAME ${MTE_NAME})
    endif ()

    # Plug MiroTypeExportMain in as a source so the executable is non-empty
    # and main() is unconditionally present.
    add_executable(${MTE_NAME} $<TARGET_OBJECTS:MiroTypeExportMain>)
    target_link_libraries(${MTE_NAME} PRIVATE MiroTypeExportMain)

    # SOURCES paths are resolved against CMAKE_CURRENT_SOURCE_DIR at call
    # site (the caller's CMakeLists.txt directory), so users can pass
    # bare filenames like "Registrations.cpp".
    if (MTE_SOURCES)
        target_sources(${MTE_NAME} PRIVATE ${MTE_SOURCES})
    endif ()

    foreach (lib IN LISTS MTE_LIBRARIES)
        get_target_property(libType ${lib} TYPE)
        if (libType STREQUAL "INTERFACE_LIBRARY")
            target_link_libraries(${MTE_NAME} PRIVATE ${lib})
        else ()
            target_link_libraries(${MTE_NAME} PRIVATE
                "$<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>")
        endif ()
    endforeach ()

    set(formatArgs "")
    foreach (fmt IN LISTS MTE_FORMATS)
        list(APPEND formatArgs --format ${fmt})
    endforeach ()

    add_custom_command(TARGET ${MTE_NAME} POST_BUILD
        COMMAND ${MTE_NAME}
                --out ${MTE_OUTPUT_DIR}
                --name ${MTE_OUTPUT_NAME}
                ${formatArgs}
        COMMENT "Exporting types: ${MTE_NAME} -> ${MTE_OUTPUT_DIR}"
        VERBATIM
    )
endfunction()
