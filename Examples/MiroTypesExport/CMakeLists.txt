miro_add_type_export(
    NAME       MiroTypesExport
    SOURCES    Registrations.cpp
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    FORMATS     zod ts jsonschema cpp cpp-miro
)

# miro_add_type_export() is a no-op when cross-compiling, so the
# target only exists on host builds.
if (TARGET MiroTypesExport)
    target_link_libraries(MiroTypesExport PRIVATE miro_warnings)
endif ()
