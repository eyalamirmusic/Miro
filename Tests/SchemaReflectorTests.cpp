// Tests for SchemaReflector — proves the same MIRO_REFLECT-annotated type
// can describe its own structure as JSON Schema.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

using namespace nano;

namespace
{

struct Address
{
    std::string street;
    std::string zip;

    MIRO_REFLECT(street, zip)
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

    MIRO_REFLECT(name, age, active, address, tags, counters, note)
};

struct NestedArrays
{
    std::vector<std::vector<int>> grid;

    MIRO_REFLECT(grid)
};

struct AllOptional
{
    std::optional<int> a;
    std::optional<std::string> b;

    MIRO_REFLECT(a, b)
};

struct FixedAndDynamic
{
    std::array<int, 3> fixed;
    std::vector<int> dynamic;

    MIRO_REFLECT(fixed, dynamic)
};

enum class Color
{
    Red,
    Green,
    Blue
};

struct WithEnum
{
    Color color = Color::Red;
    std::optional<Color> accent;

    MIRO_REFLECT(color, accent)
};

} // namespace

auto schemaForPrimitiveStruct = test("Schema: struct with primitive fields") = []
{
    auto schema = Miro::schemaOf<Address>();

    check(schema["type"].asString() == "object");
    check(schema["properties"]["street"]["type"].asString() == "string");
    check(schema["properties"]["zip"]["type"].asString() == "string");
};

auto schemaForNestedStruct = test("Schema: nested struct field") = []
{
    auto schema = Miro::schemaOf<User>();

    check(schema["properties"]["address"]["type"].asString() == "object");
    check(schema["properties"]["address"]["properties"]["street"]["type"].asString()
          == "string");
};

auto schemaForVector = test("Schema: vector field") = []
{
    auto schema = Miro::schemaOf<User>();

    check(schema["properties"]["tags"]["type"].asString() == "array");
    check(schema["properties"]["tags"]["items"]["type"].asString() == "string");
};

auto schemaForMap = test("Schema: map field") = []
{
    auto schema = Miro::schemaOf<User>();

    check(schema["properties"]["counters"]["type"].asString() == "object");
    check(schema["properties"]["counters"]["additionalProperties"]["type"].asString()
          == "integer");
};

auto schemaForOptional = test("Schema: optional field marked nullable") = []
{
    auto schema = Miro::schemaOf<User>();

    check(schema["properties"]["note"]["type"].asString() == "string");
    check(schema["properties"]["note"]["nullable"].asBool() == true);
};

auto schemaForPrimitiveTypes = test("Schema: primitive type spellings") = []
{
    auto schema = Miro::schemaOf<User>();

    check(schema["properties"]["name"]["type"].asString() == "string");
    check(schema["properties"]["age"]["type"].asString() == "integer");
    check(schema["properties"]["active"]["type"].asString() == "boolean");
};

auto schemaForTopLevelVector = test("Schema: top-level vector") = []
{
    auto schema = Miro::schemaOf<std::vector<int>>();

    check(schema["type"].asString() == "array");
    check(schema["items"]["type"].asString() == "integer");
};

auto schemaForNestedVectors = test("Schema: vector of vector") = []
{
    auto schema = Miro::schemaOf<NestedArrays>();

    check(schema["properties"]["grid"]["type"].asString() == "array");
    check(schema["properties"]["grid"]["items"]["type"].asString() == "array");
    check(schema["properties"]["grid"]["items"]["items"]["type"].asString()
          == "integer");
};

auto schemaPrintRoundtrip = test("Schema: prints + reparses as valid JSON") = []
{
    auto text = Miro::schemaOfAsString<User>();
    auto reparsed = Miro::Json::parse(text);

    check(reparsed.isObject());
    check(reparsed["properties"]["name"]["type"].asString() == "string");
};

auto schemaPaintsSaveLoadStillWorks =
    test("Schema reflector does not interfere with toJSON/fromJSON") = []
{
    // Sanity: existing JSON path still works for the same type.
    auto user = User {.name = "ada", .age = 36, .tags = {"x"}};
    auto restored = Miro::createFromJSONString<User>(Miro::toJSONString(user));

    check(restored.name == "ada");
    check(restored.age == 36);
    check(restored.tags.size() == 1);
};

auto schemaForEnumField = test("Schema: enum field gets type+enum keywords") = []
{
    auto schema = Miro::schemaOf<WithEnum>();
    auto& color = schema["properties"]["color"];

    check(color["type"].asString() == "string");
    check(color["enum"].isArray());

    auto& values = color["enum"].asArray();
    check(values.size() == 3);
    check(values[0].asString() == "Red");
    check(values[1].asString() == "Green");
    check(values[2].asString() == "Blue");
};

auto schemaForOptionalEnum =
    test("Schema: optional enum keeps enum keyword and adds nullable") = []
{
    auto schema = Miro::schemaOf<WithEnum>();
    auto& accent = schema["properties"]["accent"];

    check(accent["type"].asString() == "string");
    check(accent["enum"].isArray());
    check(accent["enum"].asArray().size() == 3);
    check(accent["nullable"].asBool() == true);
};

auto schemaForTopLevelEnum =
    test("Schema: top-level enum produces type+enum schema") = []
{
    auto schema = Miro::schemaOf<Color>();

    check(schema["type"].asString() == "string");
    check(schema["enum"].isArray());
    check(schema["enum"].asArray().size() == 3);
};

auto schemaRequiredListsNonOptionalFields =
    test("Schema: 'required' lists every non-optional field") = []
{
    auto schema = Miro::schemaOf<User>();
    auto& required = schema["required"].asArray();

    auto names = std::vector<std::string> {};
    for (auto& v: required)
        names.push_back(v.asString());

    auto has = [&](std::string_view key)
    {
        for (auto& n: names)
            if (n == key)
                return true;
        return false;
    };

    check(has("name"));
    check(has("age"));
    check(has("active"));
    check(has("address"));
    check(has("tags"));
    check(has("counters"));
    check(!has("note"));
};

auto schemaRequiredOmittedWhenAllOptional =
    test("Schema: 'required' is omitted when every field is optional") = []
{
    auto schema = Miro::schemaOf<AllOptional>();
    auto& obj = schema.asObject();

    check(obj.find("required") == obj.end());
};

auto schemaRequiredPropagatesToNestedStructs =
    test("Schema: nested struct gets its own 'required' array") = []
{
    auto schema = Miro::schemaOf<User>();
    auto& addressRequired = schema["properties"]["address"]["required"].asArray();

    check(addressRequired.size() == 2);
    check(addressRequired[0].asString() == "street");
    check(addressRequired[1].asString() == "zip");
};

auto schemaRequiredInsideOptionalStruct =
    test("Schema: inner fields stay required even when the wrapping struct "
         "is optional") = []
{
    struct WithShipping
    {
        std::optional<Address> shipping;

        MIRO_REFLECT(shipping)
    };

    auto schema = Miro::schemaOf<WithShipping>();
    auto& shipping = schema["properties"]["shipping"];

    check(shipping["nullable"].asBool() == true);
    check(shipping["required"].asArray().size() == 2);
};

auto schemaMapsHaveNoRequired =
    test("Schema: map slots do not get a 'required' entry") = []
{
    auto schema = Miro::schemaOf<User>();
    auto& counters = schema["properties"]["counters"].asObject();

    check(counters.find("required") == counters.end());
};

auto schemaArrayBoundsForFixedSize =
    test("Schema: std::array<T, N> sets minItems and maxItems to N") = []
{
    auto schema = Miro::schemaOf<FixedAndDynamic>();
    auto& fixed = schema["properties"]["fixed"];

    check(fixed["type"].asString() == "array");
    check(fixed["minItems"].asNumber() == 3);
    check(fixed["maxItems"].asNumber() == 3);
};

auto schemaArrayBoundsAbsentForVector =
    test("Schema: std::vector leaves minItems/maxItems unset") = []
{
    auto schema = Miro::schemaOf<FixedAndDynamic>();
    auto& dynamic = schema["properties"]["dynamic"].asObject();

    check(dynamic.find("minItems") == dynamic.end());
    check(dynamic.find("maxItems") == dynamic.end());
};

auto schemaArrayBoundsForTopLevelArray =
    test("Schema: top-level std::array sets bounds on the root schema") = []
{
    auto schema = Miro::schemaOf<std::array<int, 7>>();

    check(schema["minItems"].asNumber() == 7);
    check(schema["maxItems"].asNumber() == 7);
};
