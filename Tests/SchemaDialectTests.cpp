// JSON Schema dialects: the default (standard) output is covered by
// SchemaReflectorTests and friends; these pin down openAiStrict() —
// closed objects, every property required, null as a type union, and
// tagged unions as an anyOf of flattened closed arms — by comparing
// whole JSON documents.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

namespace SdEnums
{

enum class SdLevel
{
    low = 1,
    high = 5
};

} // namespace SdEnums

MIRO_ENUM_AS_INTEGER(SdEnums::SdLevel)

namespace SdEnums
{

struct SdLevelHolder
{
    SdLevel level = SdLevel::low;

    MIRO_REFLECT(level)
};

} // namespace SdEnums

namespace
{

enum class SdColor
{
    red,
    green
};

struct SdInner
{
    int x = 0;

    MIRO_REFLECT(x)
};

struct SdCircle
{
    static constexpr auto miroTag = "circle";

    double radius = 0.0;

    MIRO_REFLECT(radius)
};

struct SdSquare
{
    static constexpr auto miroTag = "square";

    double side = 0.0;

    MIRO_REFLECT(side)
};

using SdShape = TaggedVariant<"kind", SdCircle, SdSquare>;

struct SdAll
{
    std::string name;
    int count = 0;
    std::optional<std::string> nick;
    Omittable<int> limit;
    SdColor color = SdColor::red;
    std::optional<SdColor> maybeColor;
    std::vector<std::string> tags;
    std::optional<std::vector<int>> maybeTags;
    SdInner inner;
    std::optional<SdInner> maybeInner;
    SdShape shape;

    void reflect(Reflector& ref)
    {
        MIRO_FIELDS(ref, name, count, nick, limit, color, maybeColor, tags);
        ref["maybeTags"](maybeTags);
        ref["inner"].doc("The inner value")(inner);
        ref["maybeInner"].doc("Maybe an inner value")(maybeInner);
        MIRO_FIELDS(ref, shape);
    }
};

struct SdButton
{
    std::string label;

    MIRO_REFLECT(label)
};

struct SdMenu
{
    std::vector<std::string> options;

    MIRO_REFLECT(options)
};

// Plain keys written next to the discriminator by the same reflect()
// body — they join every flattened arm.
struct SdInteraction
{
    std::string id;
    std::variant<SdButton, SdMenu> data;

    void reflect(Reflector& ref)
    {
        ref["id"](id);

        reflectTagged(ref,
                      "type",
                      data,
                      [](auto& d)
                      {
                          d.template alt<SdButton>(2);
                          d.template alt<SdMenu>(3);
                      });
    }
};

struct SdWithMap
{
    std::map<std::string, int> extras;

    MIRO_REFLECT(extras)
};

struct SdHoldsMap
{
    std::vector<SdWithMap> items;

    MIRO_REFLECT(items)
};

struct SdWithAny
{
    int id = 0;
    JSON payload;

    MIRO_REFLECT(id, payload)
};

struct SdEmpty
{
    void reflect(Reflector&) {}
};

std::string thrownMessage(const std::function<void()>& func)
{
    try
    {
        func();
    }
    catch (const std::invalid_argument& e)
    {
        return e.what();
    }

    return {};
}

} // namespace

auto sdStrictWholeDocument =
    test("Schema dialect: openAiStrict renders every field shape") = []
{
    auto schema = schemaOf<SdAll>(Schema::Dialect::openAiStrict());

    auto expected = Json::parse(R"({
        "$ref": "#/$defs/SdAll",
        "$defs": {
            "SdAll": {
                "type": "object",
                "additionalProperties": false,
                "properties": {
                    "name": {"type": "string"},
                    "count": {"type": "integer"},
                    "nick": {"type": ["string", "null"]},
                    "limit": {"type": ["integer", "null"]},
                    "color": {"$ref": "#/$defs/SdColor"},
                    "maybeColor": {
                        "anyOf": [{"$ref": "#/$defs/SdColor"}, {"type": "null"}]
                    },
                    "tags": {"type": "array", "items": {"type": "string"}},
                    "maybeTags": {
                        "type": ["array", "null"],
                        "items": {"type": "integer"}
                    },
                    "inner": {
                        "$ref": "#/$defs/SdInner",
                        "description": "The inner value"
                    },
                    "maybeInner": {
                        "anyOf": [{"$ref": "#/$defs/SdInner"}, {"type": "null"}],
                        "description": "Maybe an inner value"
                    },
                    "shape": {
                        "anyOf": [
                            {
                                "type": "object",
                                "additionalProperties": false,
                                "properties": {
                                    "radius": {"type": "number"},
                                    "kind": {"type": "string", "enum": ["circle"]}
                                },
                                "required": ["radius", "kind"]
                            },
                            {
                                "type": "object",
                                "additionalProperties": false,
                                "properties": {
                                    "side": {"type": "number"},
                                    "kind": {"type": "string", "enum": ["square"]}
                                },
                                "required": ["side", "kind"]
                            }
                        ]
                    }
                },
                "required": ["name", "count", "nick", "limit", "color",
                             "maybeColor", "tags", "maybeTags", "inner",
                             "maybeInner", "shape"]
            },
            "SdColor": {"type": "string", "enum": ["red", "green"]},
            "SdInner": {
                "type": "object",
                "additionalProperties": false,
                "properties": {"x": {"type": "integer"}},
                "required": ["x"]
            },
            "SdCircle": {
                "type": "object",
                "additionalProperties": false,
                "properties": {"radius": {"type": "number"}},
                "required": ["radius"]
            },
            "SdSquare": {
                "type": "object",
                "additionalProperties": false,
                "properties": {"side": {"type": "number"}},
                "required": ["side"]
            }
        }
    })");

    check(schema == expected);
};

auto sdStandardUnchanged =
    test("Schema dialect: standard() is the default output") = []
{
    auto byDefault = schemaOf<SdAll>();
    auto standard = schemaOf<SdAll>(Schema::Dialect::standard());

    check(byDefault == standard);

    auto& props = byDefault["$defs"]["SdAll"]["properties"];
    check(props["nick"]["nullable"].asBool());
    check(props["nick"]["type"].asString() == "string");
    check(!byDefault["$defs"]["SdAll"].asObject().contains("additionalProperties"));
    check(byDefault["$defs"]["SdAll"]["required"]
          == Json::parse(R"(["name", "count", "color", "tags", "inner", "shape"])"));
};

auto sdStrictIntegerTagsAndSharedFields =
    test("Schema dialect: anyOf arms carry integer tags and shared fields") = []
{
    auto schema = schemaOf<SdInteraction>(Schema::Dialect::openAiStrict());

    auto expected = Json::parse(R"({
        "anyOf": [
            {
                "type": "object",
                "additionalProperties": false,
                "properties": {
                    "id": {"type": "string"},
                    "label": {"type": "string"},
                    "type": {"type": "integer", "enum": [2]}
                },
                "required": ["id", "label", "type"]
            },
            {
                "type": "object",
                "additionalProperties": false,
                "properties": {
                    "id": {"type": "string"},
                    "options": {"type": "array", "items": {"type": "string"}},
                    "type": {"type": "integer", "enum": [3]}
                },
                "required": ["id", "options", "type"]
            }
        ]
    })");

    check(schema["$defs"]["SdInteraction"] == expected);
    check(!contains(Json::print(schema), "allOf"));
    check(!contains(Json::print(schema), "oneOf"));
};

auto sdStrictEmptyObject =
    test("Schema dialect: an empty object still states required") = []
{
    auto schema = schemaOf<SdEmpty>(Schema::Dialect::openAiStrict());

    auto expected = Json::parse(R"({
        "type": "object",
        "additionalProperties": false,
        "properties": {},
        "required": []
    })");

    check(schema["$defs"]["SdEmpty"] == expected);
};

auto sdStrictRejectsMap =
    test("Schema dialect: closed objects reject a map, naming the field") = []
{
    auto message =
        thrownMessage([] { schemaOf<SdWithMap>(Schema::Dialect::openAiStrict()); });
    check(contains(message, "SdWithMap.extras"));
    check(contains(message, "map"));

    auto nested =
        thrownMessage([] { schemaOf<SdHoldsMap>(Schema::Dialect::openAiStrict()); });
    check(contains(nested, "SdWithMap.extras"));

    // The default dialect still renders it.
    check(schemaOf<SdWithMap>()["$defs"]["SdWithMap"]["properties"]["extras"]
                               ["additionalProperties"]["type"]
                                   .asString()
          == "integer");
};

auto sdStrictRejectsAny =
    test("Schema dialect: closed objects reject a raw JSON field") = []
{
    auto message =
        thrownMessage([] { schemaOf<SdWithAny>(Schema::Dialect::openAiStrict()); });
    check(contains(message, "SdWithAny.payload"));
};

auto sdStrictNoExtensions =
    test("Schema dialect: openAiStrict drops x-enumNames") = []
{
    auto standard = schemaOf<SdEnums::SdLevelHolder>();
    auto strict = schemaOf<SdEnums::SdLevelHolder>(Schema::Dialect::openAiStrict());

    check(standard["$defs"]["SdLevel"].asObject().contains("x-enumNames"));

    auto expected = Json::parse(R"({"type": "integer", "enum": [1, 5]})");
    check(strict["$defs"]["SdLevel"] == expected);
};
