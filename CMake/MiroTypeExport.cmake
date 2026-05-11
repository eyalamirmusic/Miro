
# miro_export — declare a build-time schema target.
#
# Compiles MIRO_EXPORT_TYPES / MIRO_EXPORT_COMMAND registrations into
# a STATIC library named ${NAME} (or an INTERFACE library if no
# SOURCES are given), and a sibling executable ${NAME}_codegen that
# walks the registry to emit generated artifacts. ${NAME} is the
# user-facing target consumers link against — they get the generated
# headers via INTERFACE include dirs and the registrations via the
# library's compiled objects (link with WHOLE_ARCHIVE to preserve the
# static initializers).
#
# The single-call shortcut: pass OUTPUT_DIR + FORMATS to miro_export()
# directly and an emit step is added implicitly.
#
# Each non-INTERFACE library in LIBRARIES is linked with WHOLE_ARCHIVE
# so registrations from those libraries' static initializers survive
# linking into the codegen executable. INTERFACE libraries are linked
# plainly — their transitive sources end up compiled directly into the
# codegen executable, so there's nothing to whole-archive.
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

    # Public-facing library that consumers link. STATIC if there are
    # sources to compile (so static initializers in MIRO_EXPORT_*
    # registrations survive WHOLE_ARCHIVE linking); INTERFACE otherwise
    # (registrations come from LIBRARIES, which the codegen executable
    # also pulls in below).
    if (ME_SOURCES)
        add_library(${NAME} STATIC ${ME_SOURCES})
        target_link_libraries(${NAME} PUBLIC Miro)
    else ()
        add_library(${NAME} INTERFACE)
        target_link_libraries(${NAME} INTERFACE Miro)
    endif ()

    # The codegen executable runs as a build-time tool on the host.
    # When cross-compiling there's no way to run a foreign-arch
    # executable on the build machine, so emit a no-op stub target —
    # caller-side add_dependencies(consumer ${NAME}_codegen) keeps
    # working across host and cross builds; generated files are
    # expected to be committed to the repo for cross builds.
    if (CMAKE_CROSSCOMPILING)
        add_custom_target(${NAME}_codegen)
        return()
    endif ()

    # Plug MiroTypeExportMain in as an OBJECT source so the executable
    # is non-empty and main() is unconditionally present.
    add_executable(${NAME}_codegen $<TARGET_OBJECTS:MiroTypeExportMain>)
    target_link_libraries(${NAME}_codegen PRIVATE MiroTypeExportMain)

    if (TARGET miro_warnings)
        target_link_libraries(${NAME}_codegen PRIVATE miro_warnings)
    endif ()

    # The codegen exec links the schema library with WHOLE_ARCHIVE so
    # the registrations' static initializers fire when the runner walks
    # the registry. STATIC archives need WHOLE_ARCHIVE; an INTERFACE
    # library has nothing to whole-archive (its sources ride in via the
    # transitive PRIVATE link below).
    if (ME_SOURCES)
        target_link_libraries(${NAME}_codegen PRIVATE
            "$<LINK_LIBRARY:WHOLE_ARCHIVE,${NAME}>")
    else ()
        target_link_libraries(${NAME}_codegen PRIVATE ${NAME})
    endif ()

    foreach (lib IN LISTS ME_LIBRARIES)
        get_target_property(libType ${lib} TYPE)
        if (libType STREQUAL "INTERFACE_LIBRARY")
            target_link_libraries(${NAME}_codegen PRIVATE ${lib})
        else ()
            target_link_libraries(${NAME}_codegen PRIVATE
                "$<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>")
        endif ()
    endforeach ()

    # Note: the library does NOT depend on ${NAME}_codegen — that
    # would form a cycle (codegen links the library WHOLE_ARCHIVE so
    # the registrations' static initializers fire, and CMake only
    # tolerates dependency cycles when every node is a STATIC library).
    # The "consumer must wait for generated headers" constraint is
    # expressed at the consumer site instead (see miro_target_uses_schema
    # / eacp_target_uses_schema, which add the codegen dep directly to
    # the consuming target).

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
# Adds a POST_BUILD command that runs the codegen executable with the
# given output directory, name stem, and format list, and registers
# OUTPUT_DIR as an INTERFACE include directory on the schema library
# so consumers automatically pick up the generated headers. Multiple
# miro_export_emit() calls on the same target are supported — each
# produces its own group of files in its own dir.
#
# Usage:
#   miro_export_emit(MySchema
#       OUTPUT_DIR  <dir>
#       OUTPUT_NAME <stem>           # optional; defaults to schema target name
#       FORMATS     ts backend ...)  # optional; defaults to all
#
# Side effect: writes ${OUTPUT_DIR}/${OUTPUT_NAME}.<extension> for
# each requested format every time the codegen executable is relinked.
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

    # Make generated headers available to consumers automatically.
    target_include_directories(${NAME} INTERFACE ${MEE_OUTPUT_DIR})

    if (CMAKE_CROSSCOMPILING)
        return()
    endif ()

    set(formatArgs "")
    foreach (fmt IN LISTS MEE_FORMATS)
        list(APPEND formatArgs --format ${fmt})
    endforeach ()

    add_custom_command(TARGET ${NAME}_codegen POST_BUILD
        COMMAND ${NAME}_codegen
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
