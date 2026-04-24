add_library(Miro STATIC
    Miro/Detail/Any.cpp
    Miro/Detail/Json.cpp
    Miro/Detail/Parser.cpp
    Miro/Detail/Printer.cpp
    Miro/Detail/Reflector.cpp)
target_include_directories(Miro PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(Miro PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

if (MIRO_UNITY_BUILD)
    set_target_properties(Miro PROPERTIES UNITY_BUILD ON)
endif ()
