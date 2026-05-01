include(FetchContent)

FetchContent_Declare(
        NanoTest
        GIT_REPOSITORY https://github.com/eyalamirmusic/NanoTest.git
        GIT_TAG main
)

FetchContent_MakeAvailable(NanoTest)

nano_add_executable(MiroTests SOURCES JsonTests.cpp ReflectorTests.cpp CommandTableTests.cpp TARGETS Miro)

add_subdirectory(Benchmark)
