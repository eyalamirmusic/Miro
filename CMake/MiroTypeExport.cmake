# miro_export — declare a schema that's compiled, then walked at build
# time to emit generated artifacts.
#
# Produces one user-facing target plus hidden helpers:
#
#   ${NAME}              INTERFACE library carrying the generated header
#                        include dirs. Linking it gives a consumer the
#                        typed wrappers. The registration sources sit on
#                        a target property and are spliced into consumers
#                        on demand by eacp_target_uses_schema(... HANDLERS).
#
# Hidden helpers (in IDE folder "_codegen-internals/${NAME}"):
#   _${NAME}_exec        Build-time executable that walks the registry
#                        and emits files. EXCLUDE_FROM_ALL.
#   _${NAME}_codegen     ALL custom target running every emit registered
#                        for the schema via a single custom command.
#                        Consumers add_dependencies() against it (via
#                        eacp_target_uses_schema) so generated headers
#                        exist before the consumer compiles. Created at
#                        end-of-directory by a deferred finalizer once
#                        all miro_export_emit() calls have been seen.
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

    # Resolve sources to absolute paths up front. They get spliced into
    # consumers in other directories via eacp_target_uses_schema, where
    # relative paths would resolve against the wrong CMAKE_CURRENT_SOURCE_DIR.
    set(absSources "")
    foreach (src IN LISTS ME_SOURCES)
        if (IS_ABSOLUTE "${src}")
            list(APPEND absSources "${src}")
        else ()
            list(APPEND absSources "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        endif ()
    endforeach ()

    add_library(${NAME} INTERFACE)
    set_property(TARGET ${NAME} PROPERTY MIRO_SCHEMA_SOURCES ${absSources})

    # The codegen executable runs as a build-time tool on the host. When
    # cross-compiling there's no way to run a foreign-arch executable on
    # the build machine, so we skip creating it — caller-side
    # add_dependencies(consumer _${NAME}_codegen) becomes a no-op and
    # the project links against committed generated files.
    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    add_executable(_${NAME}_exec ${absSources} $<TARGET_OBJECTS:MiroTypeExportMain>)
    target_link_libraries(_${NAME}_exec PRIVATE MiroTypeExportMain)
    if (TARGET miro_warnings)
        target_link_libraries(_${NAME}_exec PRIVATE miro_warnings)
    endif ()
    set_target_properties(_${NAME}_exec PROPERTIES
            FOLDER "_codegen-internals/${NAME}"
            EXCLUDE_FROM_ALL TRUE)

    # The _codegen target and its driving custom_command are wired up by
    # _miro_export_finalize, which fires at end-of-directory once all
    # miro_export_emit() calls have populated the _MIRO_EMITS property.
    # EVAL CODE bakes ${NAME} in now; a plain DEFER CALL would defer
    # variable expansion to fire time, when the function scope is gone.
    cmake_language(EVAL CODE
            "cmake_language(DEFER CALL _miro_export_finalize ${NAME})")

    if (ME_OUTPUT_DIR)
        miro_export_emit(${NAME}
                OUTPUT_DIR ${ME_OUTPUT_DIR}
                OUTPUT_NAME ${ME_OUTPUT_NAME}
                FORMATS ${ME_FORMATS})
    endif ()
endfunction()


# miro_export_emit — register a generation step on a schema declared via
# miro_export(). Each emit contributes one group of files in one
# directory; multiple emits fold into the schema's single codegen target.
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

    # Record this emit on the schema target. The finalizer turns the
    # accumulated list into one add_custom_command with one COMMAND per
    # emit, all driving a single stamp. Encoded with pipes between
    # fields to survive CMake list parsing; formats use commas inside.
    string(REPLACE ";" "," fmtsCsv "${MEE_FORMATS}")
    set_property(TARGET ${NAME} APPEND PROPERTY
            _MIRO_EMITS "${MEE_OUTPUT_DIR}|${MEE_OUTPUT_NAME}|${fmtsCsv}")
endfunction()


# _miro_export_finalize — internal, runs deferred at end-of-directory.
# Materializes the single custom_command driving every registered emit
# and the ALL custom target gating on it. Splitting this off the
# miro_export() body is what lets a schema take any number of
# miro_export_emit() calls and still expose just one codegen target.
function(_miro_export_finalize NAME)
    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    get_target_property(emits ${NAME} _MIRO_EMITS)
    if (NOT emits)
        return()
    endif ()

    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/_${NAME}.stamp")

    set(cmds "")
    foreach (entry IN LISTS emits)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 outDir)
        list(GET parts 1 outName)
        list(GET parts 2 fmtsCsv)
        string(REPLACE "," ";" fmtsList "${fmtsCsv}")

        set(formatArgs "")
        foreach (fmt IN LISTS fmtsList)
            list(APPEND formatArgs --format ${fmt})
        endforeach ()

        list(APPEND cmds COMMAND _${NAME}_exec
                --out ${outDir} --name ${outName} ${formatArgs})
    endforeach ()

    add_custom_command(
            OUTPUT "${stamp}"
            DEPENDS $<TARGET_FILE:_${NAME}_exec>
            ${cmds}
            COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
            COMMENT "Exporting types: ${NAME}"
            VERBATIM)

    add_custom_target(_${NAME}_codegen ALL DEPENDS "${stamp}")
    set_target_properties(_${NAME}_codegen PROPERTIES
            FOLDER "_codegen-internals/${NAME}")
endfunction()
