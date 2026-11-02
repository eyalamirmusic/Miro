#include "TestTypes.h"

#include <Miro/Toml.h>

#include <NanoTest/NanoTest.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

namespace TomlTests
{

struct Dependency
{
    MIRO_REFLECT(name, version, features)

    std::string name;
    std::string version;
    std::vector<std::string> features;
};

struct Package
{
    MIRO_REFLECT(name, edition, published)

    std::string name;
    int edition = 0;
    bool published = false;
};

struct Manifest
{
    MIRO_REFLECT(package, dependencies, aliases, ratio, color, homepage, license)

    Package package;
    std::vector<Dependency> dependencies;
    std::map<std::string, std::string> aliases;
    double ratio = 0.0;
    Color color = Color::Red;
    std::optional<std::string> homepage;
    Omittable<std::string> license;
};

struct Scalars
{
    MIRO_REFLECT(count, big, ratio, flag, text)

    int count = 0;
    std::int64_t big = 0;
    double ratio = 0.0;
    bool flag = false;
    std::string text;
};

struct Grid
{
    MIRO_REFLECT(rows)

    std::vector<std::vector<int>> rows;
};

Manifest makeManifest()
{
    auto manifest = Manifest {};
    manifest.package = {.name = "lockstep", .edition = 2026, .published = true};
    manifest.dependencies = {
        {.name = "fmt", .version = "12.2.0", .features = {"header-only"}},
        {.name = "miniz", .version = "3.1.2", .features = {}},
    };
    manifest.aliases = {{"core", "eacp-core"}, {"net", "eacp-network"}};
    manifest.ratio = 0.1;
    manifest.color = Color::Blue;
    manifest.homepage = "https://example.com";
    manifest.license = std::string {"MIT"};
    return manifest;
}

template <typename ExceptionType, typename FunctionType>
std::optional<ExceptionType> exceptionFrom(FunctionType&& functionToUse)
{
    try
    {
        functionToUse();
    }
    catch (const ExceptionType& error)
    {
        return error;
    }

    return std::nullopt;
}

bool contains(const std::string& haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string::npos;
}

} // namespace TomlTests

using namespace TomlTests;

auto tomlReadsAStruct = test("TOML: reads a struct from a document") = []
{
    auto manifest = createFromTOMLString<Manifest>(R"(
ratio = 0.5
color = "Green"

[package]
name = "lockstep"
edition = 2026
published = true

[aliases]
core = "eacp-core"

[[dependencies]]
name = "fmt"
version = "12.2.0"
features = ["header-only"]

[[dependencies]]
name = "miniz"
version = "3.1.2"
)");

    check(manifest.package.name == "lockstep");
    check(manifest.package.edition == 2026);
    check(manifest.package.published);
    check(manifest.ratio == 0.5);
    check(manifest.color == Color::Green);
    check(manifest.aliases.at("core") == "eacp-core");
    check(manifest.dependencies.size() == 2);
    check(manifest.dependencies[0].features
          == std::vector<std::string> {"header-only"});
    check(manifest.dependencies[1].name == "miniz");
    check(manifest.dependencies[1].features.empty());
    check(!manifest.homepage.has_value());
    check(!manifest.license.has_value());
};

auto tomlRoundTripsAStruct = test("TOML: a written struct reads back equal") = []
{
    auto original = makeManifest();
    auto text = toTOMLString(original);
    auto copy = createFromTOMLString<Manifest>(text);

    check(copy.package.name == original.package.name);
    check(copy.package.edition == original.package.edition);
    check(copy.package.published == original.package.published);
    check(copy.dependencies.size() == original.dependencies.size());
    check(copy.dependencies[0].features == original.dependencies[0].features);
    check(copy.dependencies[1].version == original.dependencies[1].version);
    check(copy.aliases == original.aliases);
    check(copy.ratio == original.ratio);
    check(copy.color == original.color);
    check(copy.homepage == original.homepage);
    check(copy.license == original.license);
};

auto tomlWritesArraysOfTables =
    test("TOML: a vector of structs is an array of tables") = []
{
    auto text = toTOMLString(makeManifest());

    check(contains(text, "[[dependencies]]"));
    check(contains(text, "[package]"));
    check(contains(text, "color = \"Blue\""));
};

auto tomlLeavesOutNullOptionals = test("TOML: a null optional writes no key") = []
{
    auto manifest = makeManifest();
    manifest.homepage.reset();
    manifest.license.reset();
    auto text = toTOMLString(manifest);

    check(!contains(text, "homepage"));
    check(!contains(text, "license"));
};

auto tomlScalarsRoundTrip = test("TOML: every scalar type round-trips exactly") = []
{
    auto original = Scalars {.count = -7,
                             .big = 9007199254740993,
                             .ratio = 0.1,
                             .flag = true,
                             .text = "tab\there \"quoted\" \xc3\xa9"};
    auto copy = createFromTOMLString<Scalars>(toTOMLString(original));

    check(copy.count == original.count);
    check(copy.big == original.big);
    check(copy.ratio == original.ratio);
    check(copy.flag == original.flag);
    check(copy.text == original.text);
};

auto tomlNestedArraysRoundTrip = test("TOML: nested vectors round-trip") = []
{
    auto original = Grid {.rows = {{1, 2}, {}, {3}}};
    auto copy = createFromTOMLString<Grid>(toTOMLString(original));

    check(copy.rows == original.rows);
};

auto tomlReadsInlineTables = test("TOML: inline tables read as structs") = []
{
    auto manifest = createFromTOMLString<Manifest>(
        R"(package = { name = "inline", edition = 3 }
dependencies = [ { name = "a", version = "1" }, { name = "b", version = "2" } ]
)");

    check(manifest.package.name == "inline");
    check(manifest.package.edition == 3);
    check(manifest.dependencies.size() == 2);
    check(manifest.dependencies[1].version == "2");
};

auto tomlIgnoresExtraKeys = test("TOML: unknown keys are ignored, as in JSON") = []
{
    auto package = createFromTOMLString<Package>(R"(
name = "x"
unknown = 1
[unrelated]
value = true
)");

    check(package.name == "x");
};

auto tomlMissingKeysKeepTheirValues =
    test("TOML: missing keys keep their prior values, as in JSON") = []
{
    auto package = Package {.name = "kept", .edition = 5, .published = true};
    fromTOMLString(package, "edition = 6\n");

    check(package.name == "kept");
    check(package.edition == 6);
    check(package.published);
};

auto tomlIntegerEnums = test("TOML: an integer enum reads its underlying value") = []
{
    auto value = createFromTOMLString<ClassWithEnum>(R"(
color = "Blue"
signal = "Stop"
mode = "ModeAuto"
)");

    check(value.color == Color::Blue);
    check(value.signal == Signal::Stop);
    check(value.mode == ModeAuto);
};

auto tomlParsesIntoJsonValues =
    test("TOML: parse() yields the matching Json values") = []
{
    auto value = Toml::parse(R"(
integer = 0x10
float = 1e3
inf = inf
literal = 'C:\path'
multiline = """
two
lines"""
dotted.key = 1
)");

    check(value["integer"].isInteger());
    check(value["integer"].asInteger() == 16);
    check(value["float"].asNumber() == 1000.0);
    check(value["literal"].asString() == "C:\\path");
    check(value["multiline"].asString() == "two\nlines");
    check(value["dotted"]["key"].asInteger() == 1);
};

auto tomlSyntaxErrorsReportTheLine = test("TOML: a syntax error names its line") = []
{
    auto error =
        exceptionFrom<Toml::ParseError>([] { Toml::parse("a = 1\nb = \n"); });

    check(error.has_value());
    check(error->line == 2);
    check(error->keyPath.empty());
    check(contains(error->what(), "TOML parse error at line 2, column "));
};

auto tomlDuplicateKeysReportTheLine =
    test("TOML: a duplicate key names its line") = []
{
    auto error = exceptionFrom<Toml::ParseError>(
        [] { Toml::parse("[a]\nx = 1\n\n[a]\ny = 2\n"); });

    check(error.has_value());
    check(error->line == 4);
};

auto tomlDatesReportKeyPathAndLine =
    test("TOML: a date names its key path and line, and is refused") = []
{
    auto error = exceptionFrom<Toml::ParseError>(
        []
        { Toml::parse("[[deps]]\nname = \"a\"\n[[deps]]\nwhen = 1979-05-27\n"); });

    check(error.has_value());
    check(error->keyPath == "deps[1].when");
    check(error->line == 4);
    check(error->what()
          == std::string {"TOML parse error at line 4, column 8, key "
                          "'deps[1].when': TOML date values are not supported, "
                          "because Miro has no date or time type to load them "
                          "into"});
};

auto tomlStructLoadThrowsOnBadInput =
    test("TOML: fromTOMLString throws ParseError for a malformed document") = []
{
    auto error = exceptionFrom<Toml::ParseError>(
        [] { (void) createFromTOMLString<Package>("name = \"unterminated\n"); });

    check(error.has_value());
    check(error->line == 1);
};

auto tomlPrintRefusesNonObjects =
    test("TOML: print() refuses a non-object root") = []
{
    auto error =
        exceptionFrom<Toml::PrintError>([] { Toml::print(Json::Value {3}); });

    check(error.has_value());
    check(error->what()
          == std::string {"TOML print error: a TOML document must be a table, so "
                          "the value to print must be an object"});
};

auto tomlPrintRefusesNullElements =
    test("TOML: print() refuses a null array element and names its path") = []
{
    auto value = Json::parse(R"({"outer": {"list": [1, null]}})");

    auto error = exceptionFrom<Toml::PrintError>([&] { Toml::print(value); });

    check(error.has_value());
    check(error->keyPath == "outer.list[1]");
};

auto tomlPrintsAValueTree =
    test("TOML: print() writes headers, quoted keys and floats") = []
{
    auto value = Json::parse(R"({
        "title": "x",
        "whole": 5.0,
        "odd key": 1,
        "server": {"port": 80, "tls": {"enabled": true}},
        "targets": [{"name": "a", "flags": {"debug": false}}, {"name": "b"}],
        "matrix": [[1, 2], [{"inline": 1}]]
    })");

    auto text = Toml::print(value);

    check(text
          == "matrix = [ [ 1, 2 ], [ { inline = 1 } ] ]\n"
             "\"odd key\" = 1\n"
             "title = \"x\"\n"
             "whole = 5.0\n"
             "\n"
             "[server]\n"
             "port = 80\n"
             "\n"
             "[server.tls]\n"
             "enabled = true\n"
             "\n"
             "[[targets]]\n"
             "name = \"a\"\n"
             "\n"
             "[targets.flags]\n"
             "debug = false\n"
             "\n"
             "[[targets]]\n"
             "name = \"b\"\n");
    check(Toml::parse(text) == value);
};

struct Workspace
{
    MIRO_REFLECT(members, dependencies)

    Vector<std::string> members;
    Vector<Dependency> dependencies;
};

auto tomlEAVectorsRoundTrip =
    test("TOML: Miro::Vector fields, of strings and of structs, round-trip") = []
{
    auto original = Workspace {};
    original.members.add("core");
    original.members.add("net");
    original.dependencies.add({.name = "fmt", .version = "12.2.0", .features = {}});
    original.dependencies.add(
        {.name = "miniz", .version = "3.1.2", .features = {"zip"}});

    auto text = toTOMLString(original);
    auto copy = createFromTOMLString<Workspace>(text);

    check(contains(text, "[[dependencies]]"));
    check(copy.members.size() == 2);
    check(copy.members[1] == "net");
    check(copy.dependencies.size() == 2);
    check(copy.dependencies[0].name == "fmt");
    check(copy.dependencies[1].features == std::vector<std::string> {"zip"});
};
