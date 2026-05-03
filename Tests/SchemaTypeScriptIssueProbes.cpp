// Probes for known/suspected gaps in the Schema reflector and the
// TypeScript exporter. Each test asserts the *ideal* behavior, so a
// failing test here documents a bug or missing feature. As fixes land,
// the probe tests move into the canonical test files alongside positive
// coverage.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <string>
#include <string_view>

using namespace nano;

namespace
{

struct Required
{
    std::string name;
    int age = 0;
    std::optional<std::string> note;

    MIRO_REFLECT(name, age, note)
};

struct FixedArr
{
    std::array<int, 5> grid;

    MIRO_REFLECT(grid)
};

namespace AlphaNs
{
struct Item
{
    int id = 0;

    MIRO_REFLECT(id)
};
} // namespace AlphaNs

namespace BetaNs
{
struct Item
{
    bool flag = false;

    MIRO_REFLECT(flag)
};
} // namespace BetaNs

struct Container
{
    AlphaNs::Item alpha;
    BetaNs::Item beta;

    MIRO_REFLECT(alpha, beta)
};

bool contains(const std::string& haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string::npos;
}

} // namespace

auto schemaRequiredArray = test(
    "Schema BUG: object schema should list 'required' for non-optional fields") = []
{
    auto schema = Miro::schemaOf<Required>();
    auto& obj = schema.asObject();

    check(obj.contains("required"));
};

auto schemaArrayBounds =
    test("Schema GAP: std::array<T, N> should set minItems and maxItems") = []
{
    auto schema = Miro::schemaOf<FixedArr>();
    auto& grid = schema["properties"]["grid"].asObject();

    check(grid.contains("minItems"));
    check(grid.contains("maxItems"));
};

auto tsNamespaceCollision = test("TypeScript BUG: types with same unqualified name "
                                 "in different namespaces collapse silently") = []
{
    auto out = Miro::TypeScript::toZod<Container>();

    // AlphaNs::Item and BetaNs::Item have different fields; the exporter
    // reduces both to "Item" by unqualified name and only emits one
    // declaration. The second namespace's shape is silently dropped.
    check(contains(out, "id: z.number().int()"));
    check(contains(out, "flag: z.boolean()"));
};
