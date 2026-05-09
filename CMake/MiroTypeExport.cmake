
# miro_export — declare a build-time schema target.
#
# Compiles MIRO_EXPORT_TYPES / MIRO_EXPORT_COMMAND registrations into
# an exporter executable named ${NAME}. By itself it produces no
# output files; pair with one or more miro_export_emit() calls to
# write generated artifacts to OUTPUT_DIR(s).
#
# The single-call shortcut: pass OUTPUT_DIR + FORMATS to miro_export()
# directly and an emit step is added implicitly.
#
# Each non-INTERFACE library in LIBRARIES is linked with WHOLE_ARCHIVE
# so the registrations' static initializers survive linking. INTERFACE
# libraries are linked plainly — their transitive sources end up
# compiled directly into the exporter, so there's nothing to
# whole-archive.
#
# Usage:
#   miro_export(MySchema
#       SOURCES     Registrations.cpp     # paths relative to caller's dir
#       LIBRARIES   MyExtraTypes          # optional
#       [OUTPUT_DIR <dir>                 # if set, also runs miro_export_emit()
#        OUTPUT_NAME <stem>               # optional; defaults to NAME
#        FORMATS     ts backend])         # optional; defaults to all
#
# At least one of SOURCES or LIBRARIES must be provided.
function(miro_export NAME)
    set(oneValueArgs OUTPUT_DIR OUTPUT_NAME)
    set(multiValueArgs FORMATS LIBRARIES SOURCES)
    cmake_parse_arguments(ME
        "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ME_SOURCES AND NOT ME_LIBRARIES)
        message(FATAL_ERROR
            "miro_export(${NAME}): at least one of SOURCES or LIBRARIES is required")
    endif ()

    # The exporter is a build-time tool that runs as a POST_BUILD step on
    # the host. When cross-compiling there's no way to run a foreign-arch
    # executable on the build machine, so emit a no-op stub target with
    # the requested name. This keeps caller-side add_dependencies(${NAME})
    # working unchanged across host and cross builds; the generated files
    # are expected to be committed to the repo for cross builds.
    if (CMAKE_CROSSCOMPILING)
        add_custom_target(${NAME})
        return()
    endif ()

    # Plug MiroTypeExportMain in as a source so the executable is non-empty
    # and main() is unconditionally present.
    add_executable(${NAME} $<TARGET_OBJECTS:MiroTypeExportMain>)
    target_link_libraries(${NAME} PRIVATE MiroTypeExportMain)

    if (TARGET miro_warnings)
        target_link_libraries(${NAME} PRIVATE miro_warnings)
    endif ()

    # SOURCES paths are resolved against CMAKE_CURRENT_SOURCE_DIR at call
    # site (the caller's CMakeLists.txt directory), so users can pass
    # bare filenames like "Registrations.cpp".
    if (ME_SOURCES)
        target_sources(${NAME} PRIVATE ${ME_SOURCES})
    endif ()

    foreach (lib IN LISTS ME_LIBRARIES)
        get_target_property(libType ${lib} TYPE)
        if (libType STREQUAL "INTERFACE_LIBRARY")
            target_link_libraries(${NAME} PRIVATE ${lib})
        else ()
            target_link_libraries(${NAME} PRIVATE
                "$<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>")
        endif ()
    endforeach ()

    if (ME_OUTPUT_DIR)
        miro_export_emit(${NAME}
            OUTPUT_DIR ${ME_OUTPUT_DIR}
            OUTPUT_NAME ${ME_OUTPUT_NAME}
            FORMATS ${ME_FORMATS})
    endif ()
endfunction()


# miro_export_emit — append a generation step to a schema target
# previously declared via miro_export().
#
# Adds a POST_BUILD command that runs the exporter executable with the
# given output directory, name stem, and format list. Multiple
# miro_export_emit() calls on the same target are supported — each
# produces its own group of files. Use this when the same set of
# registrations needs to feed several consumers (e.g. a TypeScript
# frontend in one tree and a C++ client in a build directory).
#
# Usage:
#   miro_export_emit(MySchema
#       OUTPUT_DIR  <dir>
#       OUTPUT_NAME <stem>           # optional; defaults to schema target name
#       FORMATS     ts backend ...)  # optional; defaults to all
#
# Side effect: writes ${OUTPUT_DIR}/${OUTPUT_NAME}.<extension> for
# each requested format every time the exporter is relinked.
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

    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    set(formatArgs "")
    foreach (fmt IN LISTS MEE_FORMATS)
        list(APPEND formatArgs --format ${fmt})
    endforeach ()

    add_custom_command(TARGET ${NAME} POST_BUILD
        COMMAND ${NAME}
                --out ${MEE_OUTPUT_DIR}
                --name ${MEE_OUTPUT_NAME}
                ${formatArgs}
        COMMENT "Exporting types: ${NAME} -> ${MEE_OUTPUT_DIR}"
        VERBATIM
    )
endfunction()


# miro_add_type_export — deprecated. Forwards to miro_export() +
# miro_export_emit(). Kept so existing callers continue to work; new
# code should use the two functions above.
function(miro_add_type_export)
    set(oneValueArgs NAME OUTPUT_DIR OUTPUT_NAME)
    set(multiValueArgs FORMATS LIBRARIES SOURCES)
    cmake_parse_arguments(MTE
        "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT MTE_NAME)
        message(FATAL_ERROR "miro_add_type_export: NAME is required")
    endif ()
    if (NOT MTE_OUTPUT_DIR)
        message(FATAL_ERROR "miro_add_type_export: OUTPUT_DIR is required")
    endif ()

    message(DEPRECATION
        "miro_add_type_export() is deprecated; use miro_export(${MTE_NAME} ...) "
        "and (if multiple emit groups are needed) miro_export_emit(${MTE_NAME} ...).")

    miro_export(${MTE_NAME}
        SOURCES ${MTE_SOURCES}
        LIBRARIES ${MTE_LIBRARIES}
        OUTPUT_DIR ${MTE_OUTPUT_DIR}
        OUTPUT_NAME ${MTE_OUTPUT_NAME}
        FORMATS ${MTE_FORMATS})
endfunction()
