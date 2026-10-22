// Miro::OpenAI: tool definitions built from a reflected API (whole
// definitions() documents compared against literals), function calls
// routed back through a Bridge, and the describe-time rejections.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

enum class OaUnit
{
    celsius,
    fahrenheit
};

struct OaPlace
{
    std::string city;
    std::string country;

    void reflect(Reflector& ref)
    {
        ref["city"].doc("City name")(city);
        MIRO_FIELDS(ref, country);
    }
};

struct OaForecastReq
{
    std::string city;
    std::optional<std::string> region;
    OaUnit unit = OaUnit::celsius;
    std::vector<int> days;
    OaPlace home;

    void reflect(Reflector& ref)
    {
        ref["city"].doc("City and country")(city);
        ref["region"].doc("State or province")(region);
        MIRO_FIELDS(ref, unit, days);
        ref["home"].doc("Where the user lives")(home);
    }
};

struct OaForecastRes
{
    std::string summary;

    MIRO_REFLECT(summary)
};

OaForecastRes summarize(const OaForecastReq& req)
{
    auto res = OaForecastRes {};
    res.summary = req.city + (req.unit == OaUnit::celsius ? " C" : " F")
                  + (req.region ? " " + *req.region : std::string {});
    return res;
}

struct OaLimitReq
{
    Omittable<int> limit;

    MIRO_REFLECT(limit)
};

struct OaFileList
{
    std::vector<std::string> names;

    MIRO_REFLECT(names)
};

struct OaFiles
{
    OaFileList list(const OaLimitReq& req)
    {
        auto all = std::vector<std::string> {"a.txt", "b.txt", "c.txt"};

        if (req.limit && *req.limit < static_cast<int>(all.size()))
            all.resize(static_cast<std::size_t>(*req.limit));

        return {all};
    }

    void reflect(ApiReflector& r)
    {
        using T = OaFiles;
        r.command<&T::list>(Doc {"List files"});
    }
};

struct OaWeatherApi
{
    OaForecastRes forecast(const OaForecastReq& req) { return summarize(req); }
    int ping() { return 42; }

    void reflect(ApiReflector& r)
    {
        using T = OaWeatherApi;
        r.command<&T::forecast>(Doc {"Forecast for a city"});
        r.command<&T::ping>(Doc {"Health check"});
        r.use("files", files);
    }

    OaFiles files;
};

struct OaRuntimeApi
{
    OaForecastRes forecast(const OaForecastReq& req) { return summarize(req); }

    void reset() { resets++; }

    OaForecastRes fail(const OaForecastReq&)
    {
        throw std::runtime_error("station offline");
    }

    // MethodInfo only recognises an async handler by its by-value
    // Completer<Res> parameter, so this one can't be a const ref.
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    void forecastLater(const OaForecastReq& req, Completer<OaForecastRes> done)
    {
        std::thread([req, done] { done.resolve(summarize(req)); }).detach();
    }

    void reflect(ApiReflector& r)
    {
        using T = OaRuntimeApi;
        r.commands<&T::forecast, &T::reset, &T::fail, &T::forecastLater>();
        r.use("files", files);
    }

    OaFiles files;
    int resets = 0;
};

// Request roots the strict dialect or OpenAI can't take.
struct OaButton
{
    std::string label;

    MIRO_REFLECT(label)
};

struct OaMenu
{
    std::vector<std::string> options;

    MIRO_REFLECT(options)
};

struct OaInteraction
{
    std::variant<OaButton, OaMenu> data;

    void reflect(Reflector& ref)
    {
        reflectTagged(ref,
                      "type",
                      data,
                      [](auto& d)
                      {
                          d.template alt<OaButton>(2);
                          d.template alt<OaMenu>(3);
                      });
    }
};

struct OaUnionApi
{
    void interact(const OaInteraction&) {}

    void reflect(ApiReflector& r)
    {
        using T = OaUnionApi;
        r.command<&T::interact>();
    }
};

struct OaMapReq
{
    std::map<std::string, int> extras;

    MIRO_REFLECT(extras)
};

struct OaMapApi
{
    void tag(const OaMapReq&) {}

    void reflect(ApiReflector& r)
    {
        using T = OaMapApi;
        r.command<&T::tag>();
    }
};

struct OaTreeReq
{
    std::string label;
    std::vector<OaTreeReq> children;

    MIRO_REFLECT(label, children)
};

struct OaTreeApi
{
    void plant(const OaTreeReq&) {}

    void reflect(ApiReflector& r)
    {
        using T = OaTreeApi;
        r.command<&T::plant>();
    }
};

struct OaBadNameApi
{
    int ping() { return 0; }

    void reflect(ApiReflector& r)
    {
        using T = OaBadNameApi;
        r.command<&T::ping>("ping me");
    }
};

struct OaLongNameApi
{
    int ping() { return 0; }

    void reflect(ApiReflector& r)
    {
        using T = OaLongNameApi;
        r.command<&T::ping>(std::string(65, 'p'));
    }
};

struct OaCollidingApi
{
    int ping() { return 0; }

    void reflect(ApiReflector& r)
    {
        using T = OaCollidingApi;
        r.command<&T::ping>("a.b");
        r.command<&T::ping>("a__b");
    }
};

const auto expectedWeatherTools = R"([
    {
        "type": "function",
        "name": "forecast",
        "description": "Forecast for a city",
        "strict": true,
        "parameters": {
            "type": "object",
            "additionalProperties": false,
            "properties": {
                "city": {"type": "string", "description": "City and country"},
                "region": {
                    "type": ["string", "null"],
                    "description": "State or province"
                },
                "unit": {"$ref": "#/$defs/OaUnit"},
                "days": {"type": "array", "items": {"type": "integer"}},
                "home": {
                    "type": "object",
                    "additionalProperties": false,
                    "properties": {
                        "city": {"type": "string", "description": "City name"},
                        "country": {"type": "string"}
                    },
                    "required": ["city", "country"],
                    "description": "Where the user lives"
                }
            },
            "required": ["city", "region", "unit", "days", "home"],
            "$defs": {
                "OaUnit": {"type": "string", "enum": ["celsius", "fahrenheit"]},
                "OaPlace": {
                    "type": "object",
                    "additionalProperties": false,
                    "properties": {
                        "city": {"type": "string", "description": "City name"},
                        "country": {"type": "string"}
                    },
                    "required": ["city", "country"]
                }
            }
        }
    },
    {
        "type": "function",
        "name": "ping",
        "description": "Health check",
        "strict": true,
        "parameters": {
            "type": "object",
            "properties": {},
            "required": [],
            "additionalProperties": false
        }
    },
    {
        "type": "function",
        "name": "files__list",
        "description": "List files",
        "strict": true,
        "parameters": {
            "type": "object",
            "additionalProperties": false,
            "properties": {"limit": {"type": ["integer", "null"]}},
            "required": ["limit"]
        }
    }
])";

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

OpenAI::FunctionCall makeCall(const std::string& name, const std::string& arguments)
{
    auto call = OpenAI::FunctionCall {};
    call.callId = "call_1";
    call.name = name;
    call.arguments = arguments;
    return call;
}

OpenAI::FunctionCallOutput invokeAndWait(const OpenAI::ToolSet& tools,
                                         const Bridge& bridge,
                                         const OpenAI::FunctionCall& call)
{
    auto settled = std::make_shared<std::promise<OpenAI::FunctionCallOutput>>();
    auto future = settled->get_future();

    tools.invokeAsync(bridge,
                      call,
                      [settled](OpenAI::FunctionCallOutput output)
                      { settled->set_value(std::move(output)); });

    return future.get();
}

} // namespace

auto oaDefinitions = test("OpenAI: definitions() for a documented API") = []
{
    auto api = OaWeatherApi {};
    auto tools = OpenAI::ToolSet::describe(api);

    check(tools.definitions() == Json::parse(expectedWeatherTools));
};

auto oaNonStrict = test("OpenAI: options set strict and the separator") = []
{
    auto api = OaWeatherApi {};
    auto options = OpenAI::ToolOptions {};
    options.strict = false;
    options.nameSeparator = "-";

    auto tools = OpenAI::ToolSet::describe(api, options);
    auto definitions = tools.definitions();

    check(!definitions[0]["strict"].asBool());
    check(definitions[2]["name"].asString() == "files-list");
    check(*tools.commandFor("files-list") == "files.list");
    check(tools.commandFor("files__list") == nullptr);
};

auto oaCodegenFormat =
    test("OpenAI: the openai-tools codegen format emits the same array") = []
{
    auto files = TypeExport::buildCodegen<OaWeatherApi>("schema", {"openai-tools"});

    check(files.size() == 1);
    check(contains(files[0].filename, ".tools.json"));
    check(Json::parse(files[0].contents) == Json::parse(expectedWeatherTools));
};

auto oaWireTypes = test("OpenAI: wire types use OpenAI's keys") = []
{
    auto item = Json::parse(R"({
        "type": "function_call",
        "id": "fc_1",
        "call_id": "call_9",
        "name": "forecast",
        "arguments": "{\"city\":\"Oslo\"}",
        "status": "completed"
    })");

    auto call = createFromJSON<OpenAI::FunctionCall>(item);
    check(call.callId == "call_9");
    check(call.id && *call.id == "fc_1");
    check(call.status && *call.status == "completed");
    check(call.arguments == R"({"city":"Oslo"})");
    check(toJSON(call) == item);

    auto bare = createFromJSON<OpenAI::FunctionCall>(
        Json::parse(R"({"type": "function_call", "call_id": "c", "name": "n",
                        "arguments": "{}"})"));
    check(!bare.id.has_value());
    check(!bare.status.has_value());
    check(!toJSON(bare).asObject().contains("id"));

    auto output = OpenAI::FunctionCallOutput {};
    output.callId = "call_9";
    output.output = "42";
    check(toJSON(output)
          == Json::parse(R"({"type": "function_call_output", "call_id": "call_9",
                             "output": "42"})"));
};

auto oaInvokeSync = test("OpenAI: invoke runs a sync command") = []
{
    auto api = OaRuntimeApi {};
    auto bridge = Bridge {};
    bridge.use(api);
    auto tools = OpenAI::ToolSet::describe(api);

    auto output = tools.invoke(
        bridge,
        makeCall("forecast",
                 R"({"city":"Oslo","region":null,"unit":"fahrenheit","days":[],
                     "home":{"city":"Oslo","country":"NO"}})"));

    check(output.callId == "call_1");
    check(output.type == "function_call_output");
    check(output.output == R"({"summary":"Oslo F"})");
};

auto oaInvokeVoid = test("OpenAI: a void command outputs null") = []
{
    auto api = OaRuntimeApi {};
    auto bridge = Bridge {};
    bridge.use(api);
    auto tools = OpenAI::ToolSet::describe(api);

    auto output = tools.invoke(bridge, makeCall("reset", "{}"));
    check(output.output == "null");
    check(api.resets == 1);
};

auto oaInvokeDotted = test("OpenAI: dotted names map both ways") = []
{
    auto api = OaRuntimeApi {};
    auto bridge = Bridge {};
    bridge.use(api);
    auto tools = OpenAI::ToolSet::describe(api);

    check(*tools.commandFor("files__list") == "files.list");
    check(tools.commandFor("files.list") == nullptr);

    auto all = tools.invoke(bridge, makeCall("files__list", R"({"limit":null})"));
    check(all.output == R"({"names":["a.txt","b.txt","c.txt"]})");

    auto one = tools.invoke(bridge, makeCall("files__list", R"({"limit":1})"));
    check(one.output == R"({"names":["a.txt"]})");

    auto dotted = tools.invoke(bridge, makeCall("files.list", "{}"));
    check(dotted.output == R"({"error":"unknown function 'files.list'"})");
};

auto oaInvokeErrors = test("OpenAI: invoke reports errors as output") = []
{
    auto api = OaRuntimeApi {};
    auto bridge = Bridge {};
    bridge.use(api);
    auto tools = OpenAI::ToolSet::describe(api);

    auto unknown = tools.invoke(bridge, makeCall("nope", "{}"));
    check(unknown.output == R"({"error":"unknown function 'nope'"})");
    check(unknown.callId == "call_1");

    auto malformed = tools.invoke(bridge, makeCall("forecast", "{\"city\":"));
    auto malformedJson = Json::parse(malformed.output);
    check(contains(malformedJson["error"].asString(), "invalid arguments: "));

    auto thrown = tools.invoke(bridge, makeCall("fail", R"({"city":"x"})"));
    check(thrown.output == R"({"error":"station offline"})");
};

auto oaInvokeAsync = test("OpenAI: invokeAsync runs async and sync commands") = []
{
    auto api = OaRuntimeApi {};
    auto bridge = Bridge {};
    bridge.use(api);
    auto tools = OpenAI::ToolSet::describe(api);

    auto later =
        invokeAndWait(tools,
                      bridge,
                      makeCall("forecastLater", R"({"city":"Rome","region":"RM"})"));
    check(later.callId == "call_1");
    check(later.output == R"({"summary":"Rome C RM"})");

    auto inline_ =
        invokeAndWait(tools, bridge, makeCall("forecast", R"({"city":"A"})"));
    check(inline_.output == R"({"summary":"A C"})");

    auto thrown = invokeAndWait(tools, bridge, makeCall("fail", "{}"));
    check(thrown.output == R"({"error":"station offline"})");

    auto unknown = invokeAndWait(tools, bridge, makeCall("nope", "{}"));
    check(unknown.output == R"({"error":"unknown function 'nope'"})");

    auto malformed = invokeAndWait(tools, bridge, makeCall("forecast", "[1,"));
    check(contains(malformed.output, "invalid arguments: "));
};

auto oaRejectsUnionRoot =
    test("OpenAI: a tagged-union request throws at describe time") = []
{
    auto message = thrownMessage(
        []
        {
            auto api = OaUnionApi {};
            OpenAI::ToolSet::describe(api);
        });

    check(contains(message, "'interact'"));
    check(contains(message, "tagged union"));
};

auto oaRejectsMap = test("OpenAI: a map in the request names the command") = []
{
    auto message = thrownMessage(
        []
        {
            auto api = OaMapApi {};
            OpenAI::ToolSet::describe(api);
        });

    check(contains(message, "command 'tag': "));
    check(contains(message, "OaMapReq.extras"));
};

auto oaRejectsNames = test("OpenAI: illegal or colliding tool names throw") = []
{
    auto bad = thrownMessage(
        []
        {
            auto api = OaBadNameApi {};
            OpenAI::ToolSet::describe(api);
        });
    check(contains(bad, "'ping me'"));

    auto longName = thrownMessage(
        []
        {
            auto api = OaLongNameApi {};
            OpenAI::ToolSet::describe(api);
        });
    check(contains(longName, "1 to 64"));

    auto colliding = thrownMessage(
        []
        {
            auto api = OaCollidingApi {};
            OpenAI::ToolSet::describe(api);
        });
    check(contains(colliding, "already taken"));

    check(OpenAI::toolNameFor(std::string(64, 'x')) == std::string(64, 'x'));
    check(OpenAI::toolNameFor("a.b.c") == "a__b__c");
};

auto oaRecursiveRequest =
    test("OpenAI: a recursive request keeps its own $defs entry") = []
{
    auto api = OaTreeApi {};
    auto tools = OpenAI::ToolSet::describe(api);

    auto expected = Json::parse(R"({
        "type": "object",
        "additionalProperties": false,
        "properties": {
            "label": {"type": "string"},
            "children": {"type": "array", "items": {"$ref": "#/$defs/OaTreeReq"}}
        },
        "required": ["label", "children"],
        "$defs": {
            "OaTreeReq": {
                "type": "object",
                "additionalProperties": false,
                "properties": {
                    "label": {"type": "string"},
                    "children": {
                        "type": "array",
                        "items": {"$ref": "#/$defs/OaTreeReq"}
                    }
                },
                "required": ["label", "children"]
            }
        }
    })");

    check(tools.definitions()[0]["parameters"] == expected);
};
