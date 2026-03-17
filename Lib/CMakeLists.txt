add_library(Miro STATIC Miro/Miro.cpp Miro/Json.cpp Miro/Parser.cpp)
target_include_directories(Miro PUBLIC ${CMAKE_CURRENT_LIST_DIR})