miro_export(MiroNodeExport
    SOURCES    Registrations.cpp Commands.cpp
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/node/src/generated
    OUTPUT_NAME schema
    FORMATS    ts backend bridge ts-server)
