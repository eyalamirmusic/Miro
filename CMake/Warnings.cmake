add_library(miro_warnings INTERFACE)

if (MSVC)
    target_compile_options(miro_warnings INTERFACE /W4)
else ()
    target_compile_options(miro_warnings INTERFACE
            -Wall -Wextra -Wpedantic)
endif ()
