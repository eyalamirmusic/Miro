miro_add_type_export(
    NAME       MiroTypesExport
    SOURCES    Registrations.cpp
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    FORMATS     zod ts jsonschema cpp cpp-miro
)

target_link_libraries(MiroTypesExport PRIVATE miro_warnings)