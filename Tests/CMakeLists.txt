set(CMAKE_FOLDER Tests)

find_package(NanoTest REQUIRED)

nano_add_executable(MiroTests SOURCES
        JsonTests.cpp
        JsonUnicodeEscapeTests.cpp
        JsonAccessTests.cpp
        UnicodeTests.cpp
        JsonNumberTests.cpp
        JsonNumberReflectionTests.cpp
        JsonNumberLocaleTests.cpp
        JsonObjectTests.cpp
        XmlTests.cpp
        TomlTests.cpp
        ReflectorTests.cpp
        XmlReflectorTests.cpp
        CommandTableTests.cpp
        CommandExportTests.cpp
        MethodInfoTests.cpp
        EventTests.cpp
        ApiReflectorTests.cpp
        CodegenMainTests.cpp
        VirtualReflectorTests.cpp
        SchemaReflectorTests.cpp
        SchemaDialectTests.cpp
        TypeScriptTests.cpp
        CppExportTests.cpp
        CppClientTests.cpp
        EAContainerTests.cpp
        PolymorphicTests.cpp
        TaggedUnionTests.cpp
        UserPrimitiveTests.cpp
        CustomOptionsTests.cpp
        EnumFormatTests.cpp
        RawJsonTests.cpp
        OmittableTests.cpp
        DescriptionTests.cpp
        ReflectableConceptTests.cpp
        PublicHeaders/JsonHeaderTests.cpp
        PublicHeaders/UnicodeHeaderTests.cpp
        PublicHeaders/ReflectHeaderTests.cpp
        PublicHeaders/XmlHeaderTests.cpp
        PublicHeaders/TomlHeaderTests.cpp
        PublicHeaders/BridgeHeaderTests.cpp
        PublicHeaders/CodegenHeaderTests.cpp
        PublicHeaders/OpenAIHeaderTests.cpp
        OpenAITests.cpp
        TARGETS Miro)

# MiroFormats is an OBJECT library carrying the static-init format
# registrations — linking it splices the format-registry initializers
# directly into the test runner, so buildCodegen<Apis...> tests see
# the same Format set the codegen executables do.
target_link_libraries(MiroTests PRIVATE miro_warnings MiroFormats)

# The official TOML 1.0 conformance cases, read by TomlConformanceTests.cpp.
include(CPM)
CPMAddPackage(
        NAME toml-test
        GITHUB_REPOSITORY toml-lang/toml-test
        GIT_TAG v1.5.0
        DOWNLOAD_ONLY YES)
target_sources(MiroTests PRIVATE TomlConformanceTests.cpp)
set_source_files_properties(TomlConformanceTests.cpp PROPERTIES
        COMPILE_DEFINITIONS "MIRO_TOML_TEST_DIRECTORY=\"${toml-test_SOURCE_DIR}/tests\"")

nano_add_executable(MiroRecursionTests SOURCES
        RecursionTests.cpp
        TARGETS Miro)

target_link_libraries(MiroRecursionTests PRIVATE miro_warnings)

add_subdirectory(Benchmark)
