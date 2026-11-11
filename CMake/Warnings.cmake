add_library(miro_warnings INTERFACE)

# The Clang set mirrors the strict warnings downstream projects (Modalics'
# ea_recommended_warning_flags) build Miro's headers with, so a warning they
# would hit shows up here first. Their -Wno-* suppressions are left out on
# purpose: Miro builds clean without them.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(miro_warnings INTERFACE /W4)
elseif (NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(miro_warnings INTERFACE
            -Wall -Wextra -Wpedantic
            -Wunreachable-code -Wstrict-aliasing -Wuninitialized
            -Wunused-parameter -Wconditional-uninitialized
            -Wunused-private-field -Wbool-conversion -Wextra-semi
            -Wshadow-all -Wshorten-64-to-32 -Wconversion -Wsign-compare
            -Wint-conversion -Woverloaded-virtual -Wreorder
            -Wconstant-conversion -Wsign-conversion
            -Wzero-as-null-pointer-constant -Wcast-align
            -Winconsistent-missing-destructor-override
            -Wshift-sign-overflow -Wnullable-to-nonnull-conversion
            -Wswitch-enum)
elseif (NOT MSVC)
    # GCC rejects the C++-only flags on C sources (ResEmbed generates one
    # into Miro), so those are scoped to CXX.
    set(miroGccCxxOnlyWarnings
            -Wextra-semi -Woverloaded-virtual -Wreorder
            -Wzero-as-null-pointer-constant)
    target_compile_options(miro_warnings INTERFACE
            -Wall -Wextra -Wpedantic
            -Wunreachable-code -Wstrict-aliasing -Wuninitialized
            -Wunused-parameter -Wshadow -Wconversion -Wsign-compare
            -Wsign-conversion -Wcast-align -Wswitch-enum
            "$<$<COMPILE_LANGUAGE:CXX>:${miroGccCxxOnlyWarnings}>")
endif ()
