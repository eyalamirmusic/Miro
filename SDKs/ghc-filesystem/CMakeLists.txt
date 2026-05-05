add_library(ghc_filesystem INTERFACE)
add_library(ghcFilesystem::ghc_filesystem ALIAS ghc_filesystem)

target_include_directories(ghc_filesystem INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include)
