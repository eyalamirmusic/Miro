FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

add_executable(Benchmark Main.cpp)
target_link_libraries(Benchmark PRIVATE Miro nlohmann_json::nlohmann_json)
