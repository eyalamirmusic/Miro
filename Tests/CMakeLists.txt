set(CMAKE_FOLDER Tests)

find_package(NanoTest REQUIRED)

nano_add_executable(MiroTests SOURCES
        JsonTests.cpp
        ReflectorTests.cpp
        CommandTableTests.cpp
        CommandExportTests.cpp
        MethodInfoTests.cpp
        EventTests.cpp
        VirtualReflectorTests.cpp
        SchemaReflectorTests.cpp
        TypeScriptTests.cpp
        CppExportTests.cpp
        CppClientTests.cpp
        EAContainerTests.cpp
        TARGETS Miro)

target_link_libraries(MiroTests PRIVATE miro_warnings)

nano_add_executable(MiroRecursionTests SOURCES
        RecursionTests.cpp
        TARGETS Miro)

target_link_libraries(MiroRecursionTests PRIVATE miro_warnings)

add_subdirectory(Benchmark)
