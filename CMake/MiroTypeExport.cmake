# miro_export — declare a schema that's compiled, then walked at build
# time to emit generated artifacts.
#
# Produces one user-facing target plus a tucked-away internal helper:
#
#   ${NAME}              INTERFACE library. Linking it pulls in the
#                        compiled registrations (via OBJECT lib) AND the
#                        generated-header include dirs. Use this for
#                        anything that dispatches commands at runtime.
#   ${NAME}_includes     INTERFACE library. Linking it pulls in just the
#                        generated-header include dirs. Use this for
#                        clients that consume the typed wrappers (e.g.
#                        cpp-client.h) but never need the registrations
#                        in their own binary.
#   ${NAME}_gen          Synthetic ALL target that gates on the codegen
#                        run. Consumers add_dependencies(... ${NAME}_gen)
#                        so generated headers exist before they compile.
#
# Hidden helpers under _codegen-internals/${NAME}:
#   _${NAME}_objects     OBJECT library compiling SOURCES.
#   _${NAME}_exec        Executable composed from MiroTypeExportMain +
#                        _${NAME}_objects. EXCLUDE_FROM_ALL; reached only
#                        via ${NAME}_gen's custom command.
#
# Single-call shortcut: pass OUTPUT_DIR + FORMATS to miro_export() and an
# emit step is added implicitly. Call miro_export_emit() afterwards for
# additional emits (e.g. cpp headers into a different directory).
#
# Usage:
#   miro_export(MySchema
#       SOURCES     Registrations.cpp     # paths relative to caller's dir
#       [OUTPUT_DIR <dir>                 # if set, also calls miro_export_emit()
#        OUTPUT_NAME <stem>               # optional; defaults to NAME
#        FORMATS     ts backend])         # optional; defaults to all
function(miro_export NAME)
    set(oneValueArgs OUTPUT_DIR OUTPUT_NAME)
    set(multiValueArgs FORMATS SOURCES)
    cmake_parse_arguments(ME "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ME_SOURCES)
        message(FATAL_ERROR "miro_export(${NAME}): SOURCES is required")
    endif ()

    add_library(${NAME}_includes INTERFACE)
    add_library(${NAME} INTERFACE)
    target_link_libraries(${NAME} INTERFACE ${NAME}_includes)

    add_library(_${NAME}_objects OBJECT ${ME_SOURCES})
    target_link_libraries(_${NAME}_objects PUBLIC Miro)
    set_target_properties(_${NAME}_objects PROPERTIES
            FOLDER "_codegen-internals/${NAME}")

    # OBJECT-library linkage means consumers get the .o files
    # unconditionally — the static initializers behind MIRO_EXPORT_TYPES /
    # MIRO_EXPORT_COMMAND survive into the consumer's binary without any
    # WHOLE_ARCHIVE plumbing.
    target_link_libraries(${NAME} INTERFACE _${NAME}_objects)

    add_custom_target(${NAME}_gen ALL)
    set_target_properties(${NAME}_gen PROPERTIES
            FOLDER "_codegen-internals/${NAME}")

    # The codegen executable runs as a build-time tool on the host. When
    # cross-compiling there's no way to run a foreign-arch executable on
    # the build machine, so we skip creating it — caller-side
    # add_dependencies(consumer ${NAME}_gen) becomes a no-op dependency
    # and the project links against committed generated files.
    if (NOT CMAKE_CROSSCOMPILING)
        add_executable(_${NAME}_exec $<TARGET_OBJECTS:MiroTypeExportMain>)
        target_link_libraries(_${NAME}_exec PRIVATE
                MiroTypeExportMain
                _${NAME}_objects)
        if (TARGET miro_warnings)
            target_link_libraries(_${NAME}_exec PRIVATE miro_warnings)
        endif ()
        set_target_properties(_${NAME}_exec PROPERTIES
                FOLDER "_codegen-internals/${NAME}"
                EXCLUDE_FROM_ALL TRUE)
    endif ()

    if (ME_OUTPUT_DIR)
        miro_export_emit(${NAME}
                OUTPUT_DIR ${ME_OUTPUT_DIR}
                OUTPUT_NAME ${ME_OUTPUT_NAME}
                FORMATS ${ME_FORMATS})
    endif ()
endfunction()


# miro_export_emit — append a generation step to a schema target
# previously declared via miro_export(). Each emit produces one group of
# files in one directory; multiple emits on the same schema are
# supported and each feeds its own stamp into ${NAME}_gen.
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

    target_include_directories(${NAME}_includes INTERFACE ${MEE_OUTPUT_DIR})

    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    set(formatArgs "")
    foreach (fmt IN LISTS MEE_FORMATS)
        list(APPEND formatArgs --format ${fmt})
    endforeach ()

    # Disambiguate stamp + sub-target names so repeated miro_export_emit()
    # calls on the same schema don't collide.
    get_property(emitIndex DIRECTORY PROPERTY _${NAME}_emit_count)
    if (NOT emitIndex)
        set(emitIndex 0)
    endif ()
    math(EXPR nextIndex "${emitIndex} + 1")
    set_property(DIRECTORY PROPERTY _${NAME}_emit_count ${nextIndex})

    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/_${NAME}_emit_${emitIndex}.stamp")
    set(subTarget "_${NAME}_gen_emit_${emitIndex}")

    file(MAKE_DIRECTORY "${MEE_OUTPUT_DIR}")

    add_custom_command(
            OUTPUT "${stamp}"
            DEPENDS _${NAME}_exec $<TARGET_FILE:_${NAME}_exec>
            COMMAND _${NAME}_exec
                    --out ${MEE_OUTPUT_DIR}
                    --name ${MEE_OUTPUT_NAME}
                    ${formatArgs}
            COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
            COMMENT "Exporting types: ${NAME} -> ${MEE_OUTPUT_DIR}"
            VERBATIM)

    add_custom_target(${subTarget} DEPENDS "${stamp}")
    set_target_properties(${subTarget} PROPERTIES
            FOLDER "_codegen-internals/${NAME}")
    add_dependencies(${NAME}_gen ${subTarget})
endfunction()
