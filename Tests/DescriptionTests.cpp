// Descriptions through the reflection layer: Property::doc() on fields
// and Miro::Doc on commands. A field's text lands on its slot's
// TypeNode (whatever shape the slot has), a command's on the
// DescribeReflector record and the CommandEntry; the JSON Schema and
// TypeScript emitters print them, and Load/Save never notice.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

enum class DocUnit
{
    celsius,
    fahrenheit
};

struct DocPlace
{
    std::string city;

    void reflect(Reflector& ref) { ref["city"].doc("City and country")(city); }
};

struct DocForecast
{
    std::string city;
    std::optional<std::string> region;
    Omittable<int> days;
    std::vector<double> temps;
    DocUnit unit = DocUnit::celsius;
    DocPlace home;
    DocPlace away;
    int plain = 0;

    void reflect(Reflector& ref)
    {
        ref["city"].doc("City and country")(city);
        ref["region"].doc("State or province")(region);
        ref["days"].doc("Forecast length in days")(days);
        ref["temps"].doc("Hourly temperatures")(temps);
        ref["unit"].doc("Temperature unit")(unit);
        ref["home"].doc("Where the user lives")(home);
        ref["away"].doc("Where the user is travelling")(away);
        MIRO_FIELDS(ref, plain);
    }
};

const TypeTree::TypeNode& fieldNode(const TypeTree::TypeNode& node,
                                    const std::string& name)
{
    for (auto& field: node.fields)
        if (field.name == name)
            return *field.type;

    throw std::runtime_error("no field " + name);
}

struct DocWeatherReq
{
    std::string city;

    void reflect(Reflector& ref) { ref["city"].doc("City and country")(city); }
};

struct DocWeatherRes
{
    double temperature = 0.0;

    MIRO_REFLECT(temperature)
};

DocWeatherRes docFreeWeather(const DocWeatherReq&)
{
    return {};
}

struct DocWeatherApi
{
    DocWeatherRes getWeather(const DocWeatherReq&) { return {}; }
    DocWeatherRes renamed(const DocWeatherReq&) { return {}; }
    DocWeatherRes explicitName(const DocWeatherReq&) { return {}; }
    void reset() {}

    void reflect(ApiReflector& r)
    {
        using T = DocWeatherApi;
        r.command<&T::getWeather>(Doc {"Current weather for a city"});
        r.command<&T::renamed>("wire_name", Doc {"Renamed command"});
        r.command(&T::explicitName, "explicit", Doc {"Explicit pmf"});
        r.command([](const DocWeatherReq&) { return DocWeatherRes {}; },
                  "lambda",
                  Doc {"Lambda command"});
        r.command<&docFreeWeather>(Doc {"Free function"});
        r.command<&T::reset>();
    }
};

const Detail::DescribeReflector::CommandRecord&
    findRecord(const Detail::DescribeReflector& describe, const std::string& name)
{
    for (auto& c: describe.commands)
        if (c.name == name)
            return c;

    throw std::runtime_error("no command " + name);
}

} // namespace

auto docPrimitiveField = test("Description: primitive field reaches TypeNode") = []
{
    auto tree = TypeTree::buildTree<DocForecast>();

    check(fieldNode(tree, "city").description == "City and country");
    check(fieldNode(tree, "plain").description.empty());
    check(tree.description.empty());
};

auto docWrappedFields =
    test("Description: optional / omittable / vector / enum fields") = []
{
    auto tree = TypeTree::buildTree<DocForecast>();

    auto& region = fieldNode(tree, "region");
    check(region.description == "State or province");
    check(region.optional);

    auto& days = fieldNode(tree, "days");
    check(days.description == "Forecast length in days");
    check(days.omittable);

    auto& temps = fieldNode(tree, "temps");
    check(temps.description == "Hourly temperatures");
    check(temps.shape == TypeTree::TypeNode::Shape::Array);
    check(temps.inner->description.empty());

    auto& unit = fieldNode(tree, "unit");
    check(unit.description == "Temperature unit");
    check(unit.shape == TypeTree::TypeNode::Shape::Enum);
};

auto docNamedStructField =
    test("Description: named struct field keeps its own slot description") = []
{
    auto tree = TypeTree::buildTree<DocForecast>();

    auto& home = fieldNode(tree, "home");
    auto& away = fieldNode(tree, "away");

    check(home.description == "Where the user lives");
    check(away.description == "Where the user is travelling");
    check(fieldNode(home, "city").description == "City and country");
};

auto docSchemaOutput = test("Description: JSON Schema prints it on slots only") = []
{
    auto schema = schemaOf<DocForecast>();
    auto& props = schema["$defs"]["DocForecast"]["properties"];

    check(props["city"]["description"].asString() == "City and country");
    check(props["region"]["description"].asString() == "State or province");
    check(props["temps"]["description"].asString() == "Hourly temperatures");
    check(props["unit"]["description"].asString() == "Temperature unit");
    check(props["home"]["description"].asString() == "Where the user lives");
    check(props["away"]["description"].asString() == "Where the user is travelling");
    check(!props["plain"].asObject().contains("description"));

    // The shared definitions stay free of any one slot's text.
    check(!schema["$defs"]["DocPlace"].asObject().contains("description"));
    check(!schema["$defs"]["DocUnit"].asObject().contains("description"));
    check(schema["$defs"]["DocPlace"]["properties"]["city"]["description"].asString()
          == "City and country");
};

auto docTypeScriptOutput = test("Description: TypeScript emits JSDoc lines") = []
{
    auto types = TypeScript::toTypes<DocForecast>();

    check(contains(types, "    /** City and country */\n    city: string;"));
    check(contains(types, "    /** Where the user lives */\n    home: DocPlace;"));
    check(contains(types, "    /** State or province */\n    region?:"));
    check(!contains(types, "*/\n    plain"));

    auto zod = TypeScript::toZod<DocForecast>();
    check(contains(zod, "    /** City and country */\n    city: z.string(),"));
};

auto docJsDocEscape = test("Description: JSDoc text cannot close the comment") = []
{
    check(Detail::formatJsDocLine("a */ b\nc", "  ") == "  /** a *\\/ b c */\n");
    check(Detail::formatJsDocLine("", "  ").empty());
};

auto docRoundTrip = test("Description: Load/Save ignore .doc()") = []
{
    auto value = DocForecast {};
    value.city = "Paris, France";
    value.region = "IDF";
    value.days = 3;
    value.temps = {1.5, 2.5};
    value.unit = DocUnit::fahrenheit;
    value.home.city = "Lyon";
    value.plain = 7;

    auto text = toJSONString(value);
    auto loaded = DocForecast {};
    fromJSONString(loaded, text);

    check(toJSONString(loaded) == text);
    check(loaded.city == "Paris, France");
    check(loaded.region == std::optional<std::string> {"IDF"});
    check(loaded.days && *loaded.days == 3);
    check(loaded.unit == DocUnit::fahrenheit);
    check(loaded.home.city == "Lyon");
    check(!contains(text, "description"));
};

auto docCommandDescribe =
    test("Description: command Doc reaches DescribeReflector") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = DocWeatherApi {};
    api.reflect(describe);

    check(findRecord(describe, "getWeather").description
          == "Current weather for a city");
    check(findRecord(describe, "wire_name").description == "Renamed command");
    check(findRecord(describe, "explicit").description == "Explicit pmf");
    check(findRecord(describe, "lambda").description == "Lambda command");
    check(findRecord(describe, "docFreeWeather").description == "Free function");
    check(findRecord(describe, "reset").description.empty());
};

auto docCommandBind = test("Description: Bridge binding ignores Doc") = []
{
    auto api = DocWeatherApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    auto result = bridge.dispatch("getWeather", Json::parse(R"({"city":"x"})"));
    check(result["temperature"].asNumber() == 0.0);
};

auto docCommandEntryAndBackend =
    test("Description: CommandEntry and backend module carry Doc") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = DocWeatherApi {};
    api.reflect(describe);

    auto entries = TypeExport::toCommandEntries(describe.commands);

    auto found = false;
    for (auto& e: entries)
    {
        if (e.name == "getWeather")
        {
            found = true;
            check(e.description == "Current weather for a city");
        }
    }
    check(found);

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {describe.typeRoots}, entries, "schema");

    check(contains(
        out, "        /** Current weather for a city */\n        getWeather: "));
    check(contains(out, "        /** Lambda command */\n        lambda: "));
    check(!contains(out, "*/\n        reset"));
};

auto docCommandRootIndices =
    test("DescribeReflector: command records index their type roots") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = DocWeatherApi {};
    api.reflect(describe);

    auto& weather = findRecord(describe, "getWeather");
    check(weather.reqRoot == std::optional<std::size_t> {0});
    check(weather.resRoot == std::optional<std::size_t> {1});

    for (auto& c: describe.commands)
    {
        check(c.reqRoot.has_value() == c.req.present);
        check(c.resRoot.has_value() == c.res.present);

        if (c.reqRoot)
            check(describe.typeRoots[*c.reqRoot].qualifiedName
                  == c.req.qualifiedName);
        if (c.resRoot)
            check(describe.typeRoots[*c.resRoot].qualifiedName
                  == c.res.qualifiedName);
    }

    auto& reset = findRecord(describe, "reset");
    check(!reset.reqRoot.has_value());
    check(!reset.resRoot.has_value());
};
