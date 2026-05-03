# Header-only target carrying the example types. Shared by the manual
# exporter (Main.cpp) and the auto-exporter built via the helper below.
add_library(ZodExportTypes STATIC Registrations.cpp)
target_link_libraries(ZodExportTypes PUBLIC Miro)

add_executable(ZodExport Main.cpp)
target_link_libraries(ZodExport PRIVATE ZodExportTypes)

miro_add_type_export(
    NAME       ZodExportAuto
    LIBRARIES  ZodExportTypes
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/ts
)
