add_library(miro_warnings INTERFACE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(miro_warnings INTERFACE /W4)
elseif (NOT MSVC)
    target_compile_options(miro_warnings INTERFACE
            -Wall -Wextra -Wpedantic)
endif ()
