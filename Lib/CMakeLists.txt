add_library(Miro STATIC Miro/Json.cpp Miro/Parser.cpp Miro/Printer.cpp)
target_include_directories(Miro PUBLIC ${CMAKE_CURRENT_LIST_DIR})