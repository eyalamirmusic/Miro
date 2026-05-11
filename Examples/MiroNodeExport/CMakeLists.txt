miro_export(MiroNodeExport
    SOURCES    MiroNodeExport.cpp
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/node/src/generated
    OUTPUT_NAME schema
    FORMATS    ts backend bridge ts-server)
