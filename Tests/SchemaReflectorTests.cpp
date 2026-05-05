// Tests for SchemaReflector — proves the same MIRO_REFLECT-annotated type
// can describe its own structure as JSON Schema.
//
// The schema format hoists every named user-defined type into a
// top-level "$defs" map; references everywhere become {"$ref": "..."}.
// `defOf(schema, "Foo")` is a helper for navigating into the body of
// a named type's definition; everything else hangs off the root.

#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

namespace
{

// Schema-only fixtures — types that don't show up in the cross-test
// shared header because they're shaped specifically for one assertion.

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

struct AddressPair
{
    Address first;
    Address second;

    MIRO_REFLECT(first, second)
};

const JSON& defOf(const JSON& schema, const char* name)
{
    return schema["$defs"][name];
}

} // namespace

auto schemaRefAtRoot = test("Schema: a named root produces $ref + $defs entry") = []
{
    auto schema = schemaOf<Address>();

    check(schema["$ref"].asString() == "#/$defs/Address");
    check(schema["$defs"]["Address"]["type"].asString() == "object");
};

auto schemaForPrimitiveStruct = test("Schema: struct with primitive fields") = []
{
    auto schema = schemaOf<Address>();
    auto& body = defOf(schema, "Address");

    check(body["type"].asString() == "object");
    check(body["properties"]["street"]["type"].asString() == "string");
    check(body["properties"]["zip"]["type"].asString() == "string");
};

auto schemaForNestedStruct =
    test("Schema: nested struct field becomes $ref to its $defs entry") = []
{
    auto schema = schemaOf<User>();
    auto& userBody = defOf(schema, "User");

    check(userBody["properties"]["address"]["$ref"].asString() == "#/$defs/Address");
    check(defOf(schema, "Address")["properties"]["street"]["type"].asString()
          == "string");
};

auto schemaForVector = test("Schema: vector field") = []
{
    auto schema = schemaOf<User>();
    auto& userBody = defOf(schema, "User");

    check(userBody["properties"]["tags"]["type"].asString() == "array");
    check(userBody["properties"]["tags"]["items"]["type"].asString() == "string");
};

auto schemaForMap = test("Schema: map field") = []
{
    auto schema = schemaOf<User>();
    auto& counters = defOf(schema, "User")["properties"]["counters"];

    check(counters["type"].asString() == "object");
    check(counters["additionalProperties"]["type"].asString() == "integer");
};

auto schemaForOptional = test("Schema: optional field marked nullable") = []
{
    auto schema = schemaOf<User>();
    auto& note = defOf(schema, "User")["properties"]["note"];

    check(note["type"].asString() == "string");
    check(note["nullable"].asBool() == true);
};

auto schemaForPrimitiveTypes = test("Schema: primitive type spellings") = []
{
    auto schema = schemaOf<User>();
    auto& userBody = defOf(schema, "User");

    check(userBody["properties"]["name"]["type"].asString() == "string");
    check(userBody["properties"]["age"]["type"].asString() == "integer");
    check(userBody["properties"]["active"]["type"].asString() == "boolean");
};

auto schemaForTopLevelVector = test("Schema: top-level vector") = []
{
    auto schema = schemaOf<std::vector<int>>();

    // No named types reachable, so $defs is omitted.
    check(schema.asObject().find("$defs") == schema.asObject().end());
    check(schema["type"].asString() == "array");
    check(schema["items"]["type"].asString() == "integer");
};

auto schemaForNestedVectors = test("Schema: vector of vector") = []
{
    auto schema = schemaOf<NestedArrays>();
    auto& grid = defOf(schema, "NestedArrays")["properties"]["grid"];

    check(grid["type"].asString() == "array");
    check(grid["items"]["type"].asString() == "array");
    check(grid["items"]["items"]["type"].asString() == "integer");
};

auto schemaPrintRoundtrip = test("Schema: prints + reparses as valid JSON") = []
{
    auto text = schemaOfAsString<User>();
    auto reparsed = Json::parse(text);

    check(reparsed.isObject());
    check(reparsed["$defs"]["User"]["properties"]["name"]["type"].asString()
          == "string");
};

auto schemaPaintsSaveLoadStillWorks =
    test("Schema reflector does not interfere with toJSON/fromJSON") = []
{
    // Sanity: existing JSON path still works for the same type.
    auto user = User {.name = "ada", .age = 36, .tags = {"x"}};
    auto restored = createFromJSONString<User>(toJSONString(user));

    check(restored.name == "ada");
    check(restored.age == 36);
    check(restored.tags.size() == 1);
};

auto schemaForEnumField =
    test("Schema: enum field is a $ref; the body lives in $defs") = []
{
    auto schema = schemaOf<User>();

    check(defOf(schema, "User")["properties"]["color"]["$ref"].asString()
          == "#/$defs/Color");

    auto& body = defOf(schema, "Color");
    check(body["type"].asString() == "string");
    check(body["enum"].isArray());

    auto& values = body["enum"].asArray();
    check(values.size() == 3);
    check(values[0].asString() == "Red");
    check(values[1].asString() == "Green");
    check(values[2].asString() == "Blue");
};

auto schemaForOptionalEnum =
    test("Schema: optional enum field keeps the $ref and adds nullable") = []
{
    auto schema = schemaOf<User>();
    auto& accent = defOf(schema, "User")["properties"]["accent"];

    check(accent["$ref"].asString() == "#/$defs/Color");
    check(accent["nullable"].asBool() == true);
};

auto schemaForTopLevelEnum =
    test("Schema: top-level enum gets hoisted to $defs like a struct") = []
{
    auto schema = schemaOf<Color>();

    check(schema["$ref"].asString() == "#/$defs/Color");
    check(defOf(schema, "Color")["type"].asString() == "string");
    check(defOf(schema, "Color")["enum"].asArray().size() == 3);
};

auto schemaRequiredListsNonOptionalFields =
    test("Schema: 'required' lists every non-optional field") = []
{
    auto schema = schemaOf<User>();
    auto& required = defOf(schema, "User")["required"].asArray();

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
    auto schema = schemaOf<AllOptional>();
    auto& body = defOf(schema, "AllOptional").asObject();

    check(body.find("required") == body.end());
};

auto schemaRequiredPropagatesToNestedStructs =
    test("Schema: nested struct gets its own 'required' array") = []
{
    auto schema = schemaOf<User>();
    auto& addressRequired = defOf(schema, "Address")["required"].asArray();

    check(addressRequired.size() == 2);
    check(addressRequired[0].asString() == "street");
    check(addressRequired[1].asString() == "zip");
};

auto schemaRequiredInsideOptionalStruct =
    test("Schema: inner fields stay required even when the wrapping struct "
         "is optional") = []
{
    auto schema = schemaOf<User>();
    auto& shipping = defOf(schema, "User")["properties"]["shipping"];

    // The reference slot carries the nullable bit; the referenced
    // type's body still requires its inner fields.
    check(shipping["$ref"].asString() == "#/$defs/Address");
    check(shipping["nullable"].asBool() == true);
    check(defOf(schema, "Address")["required"].asArray().size() == 2);
};

auto schemaMapsHaveNoRequired =
    test("Schema: map slots do not get a 'required' entry") = []
{
    auto schema = schemaOf<User>();
    auto& counters = defOf(schema, "User")["properties"]["counters"].asObject();

    check(counters.find("required") == counters.end());
};

auto schemaArrayBoundsForFixedSize =
    test("Schema: std::array<T, N> sets minItems and maxItems to N") = []
{
    auto schema = schemaOf<FixedAndDynamic>();
    auto& fixed = defOf(schema, "FixedAndDynamic")["properties"]["fixed"];

    check(fixed["type"].asString() == "array");
    check(fixed["minItems"].asNumber() == 3);
    check(fixed["maxItems"].asNumber() == 3);
};

auto schemaArrayBoundsAbsentForVector =
    test("Schema: std::vector leaves minItems/maxItems unset") = []
{
    auto schema = schemaOf<FixedAndDynamic>();
    auto& dynamic =
        defOf(schema, "FixedAndDynamic")["properties"]["dynamic"].asObject();

    check(dynamic.find("minItems") == dynamic.end());
    check(dynamic.find("maxItems") == dynamic.end());
};

auto schemaArrayBoundsForTopLevelArray =
    test("Schema: top-level std::array sets bounds on the root schema") = []
{
    auto schema = schemaOf<std::array<int, 7>>();

    check(schema["minItems"].asNumber() == 7);
    check(schema["maxItems"].asNumber() == 7);
};

auto schemaDedupsRepeatedStructs =
    test("Schema: a struct used in multiple places appears once in $defs") = []
{
    auto schema = schemaOf<AddressPair>();
    auto& defs = schema["$defs"].asObject();

    // Address only appears once, both AddressPair fields reference it.
    check(defs.contains("Address"));
    check(defOf(schema, "AddressPair")["properties"]["first"]["$ref"].asString()
          == "#/$defs/Address");
    check(defOf(schema, "AddressPair")["properties"]["second"]["$ref"].asString()
          == "#/$defs/Address");
};
