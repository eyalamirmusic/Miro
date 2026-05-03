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
#       LIBRARIES   MyTypesRegistrations
#       OUTPUT_DIR  ${CMAKE_CURRENT_BINARY_DIR}/ts
#       OUTPUT_NAME schema      # optional; defaults to NAME
#       FORMATS     zod ts      # optional; defaults to all known formats
#   )
#
# Side effects:
#   - Creates an internal executable target ${NAME}_exporter.
#   - Creates a custom-target ${NAME} (in ALL) that depends on a stamp
#     file produced after the exporter runs successfully.
#   - One bundled file per format is written to OUTPUT_DIR, named
#     ${OUTPUT_NAME}.<extension> (e.g. schema.zod.ts, schema.ts).
function(miro_add_type_export)
    set(options "")
    set(oneValueArgs NAME OUTPUT_DIR OUTPUT_NAME)
    set(multiValueArgs FORMATS LIBRARIES)
    cmake_parse_arguments(MTE
        "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT MTE_NAME)
        message(FATAL_ERROR "miro_add_type_export: NAME is required")
    endif ()
    if (NOT MTE_OUTPUT_DIR)
        message(FATAL_ERROR "miro_add_type_export: OUTPUT_DIR is required")
    endif ()
    if (NOT MTE_LIBRARIES)
        message(FATAL_ERROR "miro_add_type_export: LIBRARIES is required")
    endif ()
    if (NOT MTE_OUTPUT_NAME)
        set(MTE_OUTPUT_NAME ${MTE_NAME})
    endif ()

    set(exeTarget ${MTE_NAME}_exporter)

    # Plug MiroTypeExportMain in as a source so the executable is non-empty
    # and main() is unconditionally present.
    add_executable(${exeTarget} $<TARGET_OBJECTS:MiroTypeExportMain>)
    target_link_libraries(${exeTarget} PRIVATE MiroTypeExportMain)

    foreach (lib IN LISTS MTE_LIBRARIES)
        get_target_property(libType ${lib} TYPE)
        if (libType STREQUAL "INTERFACE_LIBRARY")
            target_link_libraries(${exeTarget} PRIVATE ${lib})
        else ()
            target_link_libraries(${exeTarget} PRIVATE
                "$<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>")
        endif ()
    endforeach ()

    set(formatArgs "")
    foreach (fmt IN LISTS MTE_FORMATS)
        list(APPEND formatArgs --format ${fmt})
    endforeach ()

    set(stamp ${CMAKE_CURRENT_BINARY_DIR}/${MTE_NAME}.stamp)

    add_custom_command(
        OUTPUT ${stamp}
        COMMAND ${exeTarget}
                --out ${MTE_OUTPUT_DIR}
                --name ${MTE_OUTPUT_NAME}
                ${formatArgs}
        COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
        DEPENDS ${exeTarget}
        COMMENT "Exporting types: ${MTE_NAME} -> ${MTE_OUTPUT_DIR}"
        VERBATIM
    )

    add_custom_target(${MTE_NAME} ALL DEPENDS ${stamp})
endfunction()
