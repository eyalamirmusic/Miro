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
    # add_dependencies(consumer ${NAME}_Codegen) becomes a no-op and
    # the project links against committed generated files.
    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    add_executable(${NAME}_Codegen
            ${absSources} $<TARGET_OBJECTS:MiroTypeExportMain>)
    target_link_libraries(${NAME}_Codegen PRIVATE MiroTypeExportMain)
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
