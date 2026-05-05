include(FetchContent)

find_package(NanoTest REQUIRED)

set(CMAKE_FOLDER Tests)

nano_add_executable(MiroTests SOURCES
        JsonTests.cpp
        ReflectorTests.cpp
        CommandTableTests.cpp
        CommandExportTests.cpp
        VirtualReflectorTests.cpp
        SchemaReflectorTests.cpp
        TypeScriptTests.cpp
        CppExportTests.cpp
        TARGETS Miro)

target_link_libraries(MiroTests PRIVATE miro_warnings)

nano_add_executable(MiroRecursionTests SOURCES
        RecursionTests.cpp
        TARGETS Miro)

target_link_libraries(MiroRecursionTests PRIVATE miro_warnings)

add_subdirectory(Benchmark)
