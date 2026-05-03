// Tests for Miro::Cpp — emits C++ headers from the same TypeNode tree
// the TypeScript and JSON Schema renderers consume. Two flavours: a
// pure-types header with no Miro dependency, and a "with Miro" header
// that adds MIRO_REFLECT(...) lines + the umbrella include.

#include <Miro/Miro.h>
#include <Miro/Cpp/Cpp.h>
#include <NanoTest/NanoTest.h>

#include <string>
#include <string_view>

using namespace nano;

namespace
{

struct Address
{
    std::string street;
    std::string zip;

    MIRO_REFLECT(street, zip)
};

enum class Color
{
    Red,
    Green,
    Blue
};

struct User
{
    std::string name;
    int age = 0;
    bool active = true;
    Address address;
    std::vector<std::string> tags;
    std::map<std::string, int> counters;
    std::optional<std::string> note;
    std::optional<Address> shipping;
    Color favourite = Color::Red;

    MIRO_REFLECT(
        name, age, active, address, tags, counters, note, shipping, favourite)
};

bool contains(const std::string& haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string::npos;
}

bool comesBefore(const std::string& s,
                 std::string_view earlier,
                 std::string_view later)
{
    auto e = s.find(earlier);
    auto l = s.find(later);
    return e != std::string::npos && l != std::string::npos && e < l;
}

} // namespace

// ---------- Plain (Miro-free) ----------

auto cppPragmaOnce = test("Cpp: header begins with #pragma once") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(out.starts_with("#pragma once"));
};

auto cppStdIncludes = test("Cpp: pulls in std headers it needs") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(contains(out, "#include <map>"));
    check(contains(out, "#include <optional>"));
    check(contains(out, "#include <string>"));
    check(contains(out, "#include <vector>"));
};

auto cppNoMiroInclude = test("Cpp: pure variant has no <Miro/Miro.h> include") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(!contains(out, "Miro/Miro.h"));
};

auto cppNoMiroReflect = test("Cpp: pure variant emits no MIRO_REFLECT lines") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(!contains(out, "MIRO_REFLECT"));
};

auto cppPrimitiveTypes =
    test("Cpp: primitive fields use bool/int/std::string with sane defaults") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(contains(out, "std::string name;"));
    check(contains(out, "int age = 0;"));
    check(contains(out, "bool active = false;"));
};

auto cppContainerTypes = test("Cpp: vector / map / optional spellings") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(contains(out, "std::vector<std::string> tags;"));
    check(contains(out, "std::map<std::string, int> counters;"));
    check(contains(out, "std::optional<std::string> note;"));
    check(contains(out, "std::optional<Address> shipping;"));
};

auto cppNamedReference = test("Cpp: nested named struct emitted by name") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(contains(out, "Address address;"));
};

auto cppEnumDeclaration = test("Cpp: enum becomes a top-level enum class") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(contains(out, "enum class Color"));
    check(contains(out, "Red"));
    check(contains(out, "Green"));
    check(contains(out, "Blue"));
};

auto cppDependencyOrder =
    test("Cpp: dependency ordering — referenced types declared first") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/false);
    check(comesBefore(out, "struct Address", "struct User"));
    check(comesBefore(out, "enum class Color", "struct User"));
};

// ---------- With-Miro variant ----------

auto cppMiroIncludes = test("Cpp (miro): includes Miro/Miro.h") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/true);
    check(contains(out, "#include <Miro/Miro.h>"));
};

auto cppMiroReflect =
    test("Cpp (miro): every struct has a MIRO_REFLECT line listing its fields") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/true);
    check(contains(out, "MIRO_REFLECT(street, zip)"));
    check(contains(out,
                   "MIRO_REFLECT(name, age, active, address, tags, counters, "
                   "note, shipping, favourite)"));
};

auto cppMiroEnumNoReflect =
    test("Cpp (miro): enum classes do not get a MIRO_REFLECT line") = []
{
    auto out = Miro::Cpp::toHeader<User>(/*withMiro=*/true);
    // The enum block sits between "enum class Color" and the next "};"
    auto enumStart = out.find("enum class Color");
    auto enumEnd = out.find("};", enumStart);
    auto enumBlock = out.substr(enumStart, enumEnd - enumStart);
    check(!contains(enumBlock, "MIRO_REFLECT"));
};
