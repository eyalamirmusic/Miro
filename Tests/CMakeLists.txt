include(FetchContent)

FetchContent_Declare(
        NanoTest
        GIT_REPOSITORY https://github.com/eyalamirmusic/NanoTest.git
        GIT_TAG main
)

FetchContent_MakeAvailable(NanoTest)

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

# Isolated binary for tests that may stack-overflow today (recursive
# self-referencing types in Schema / TypeScript exporters). Keeping
# them out of MiroTests means a SIGSEGV in one of these doesn't take
# the rest of the suite down with it.
nano_add_executable(MiroRecursionTests SOURCES
        RecursionTests.cpp

        TARGETS Miro)

add_subdirectory(Benchmark)
