#include "OpenAI.h"

#include "../Reflection/Serialize.h"
#include "../Schema/Schema.h"

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <utility>

namespace Miro::OpenAI
{

namespace
{

constexpr auto maxToolNameLength = std::size_t {64};
constexpr auto defsPrefix = std::string_view {"#/$defs/"};

bool isToolNameChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
           || c == '_' || c == '-';
}

std::string commandError(std::string_view command, std::string_view message)
{
    auto out = std::string {"command '"};
    out += command;
    out += "': ";
    out += message;
    return out;
}

std::string errorOutput(const std::string& message)
{
    auto object = Json::Object {};
    object["error"] = message;
    return Json::print(JSON {std::move(object)});
}

JSON emptyParameters()
{
    auto object = Json::Object {};
    object["type"] = "object";
    object["properties"] = Json::Object {};
    object["required"] = Json::Array {};
    object["additionalProperties"] = false;
    return JSON {std::move(object)};
}

// The definition name a root {"$ref": "#/$defs/Name"} points at, empty
// when the root is anything else (an inline array or primitive).
std::string rootDefinitionName(const JSON& schema)
{
    if (!schema.isObject())
        return {};

    auto& object = schema.asObject();
    auto ref = object.find("$ref");

    if (ref == object.end() || !ref->second.isString())
        return {};

    auto target = std::string_view {ref->second.asString()};

    if (!target.starts_with(defsPrefix))
        return {};

    return std::string {target.substr(defsPrefix.size())};
}

bool mentions(const JSON& value, const std::string& text)
{
    return Json::print(value).find(text) != std::string::npos;
}

} // namespace

void FunctionCall::reflect(Reflector& ref)
{
    ref["type"](type);
    ref["id"](id);
    ref["call_id"](callId);
    ref["name"](name);
    ref["arguments"](arguments);
    ref["status"](status);
}

void FunctionCallOutput::reflect(Reflector& ref)
{
    ref["type"](type);
    ref["call_id"](callId);
    ref["output"](output);
}

void FunctionTool::reflect(Reflector& ref)
{
    ref["type"](type);
    ref["name"](name);
    ref["description"](description);
    ref["parameters"](parameters);
    ref["strict"](strict);
}

std::string toolNameFor(std::string_view command, std::string_view separator)
{
    auto name = std::string {};

    for (auto c: command)
    {
        if (c == '.')
            name += separator;
        else
            name += c;
    }

    if (name.empty() || name.size() > maxToolNameLength)
        throw std::invalid_argument(commandError(
            command, "tool name '" + name + "' must be 1 to 64 characters"));

    for (auto c: name)
    {
        if (!isToolNameChar(c))
            throw std::invalid_argument(commandError(
                command, "tool name '" + name + "' may only hold [A-Za-z0-9_-]"));
    }

    return name;
}

JSON parametersFor(std::string_view command, TypeTree::TypeNode* request)
{
    if (request == nullptr)
        return emptyParameters();

    auto schema = JSON {};

    try
    {
        schema = formatJsonSchema(*request, Schema::Dialect::openAiStrict());
    }
    catch (const std::invalid_argument& e)
    {
        throw std::invalid_argument(commandError(command, e.what()));
    }

    auto defs = Json::Object {};

    if (schema.isObject())
    {
        auto& object = schema.asObject();
        auto found = object.find("$defs");

        if (found != object.end())
        {
            defs = std::move(found->second.asObject());
            object.erase(found);
        }
    }

    auto body = std::move(schema);
    auto rootName = rootDefinitionName(body);

    if (!rootName.empty())
    {
        auto found = defs.find(rootName);

        if (found != defs.end())
        {
            body = found->second;
            defs.erase(found);

            // A recursive request still refers to its own definition,
            // so it stays under $defs as well as at the root.
            auto selfRef = std::string {defsPrefix} + rootName + "\"";
            auto rest = JSON {defs};

            if (mentions(body, selfRef) || mentions(rest, selfRef))
                defs[rootName] = body;
        }
    }

    auto isObject = body.isObject() && body.asObject().contains("type")
                    && body["type"].isString()
                    && body["type"].asString() == "object";

    if (body.isObject() && body.asObject().contains("anyOf"))
        throw std::invalid_argument(commandError(
            command,
            "the request is a tagged union; tool parameters must be an object"));

    if (!isObject)
        throw std::invalid_argument(
            commandError(command, "the request type must be an object"));

    if (!defs.empty())
        body.asObject()["$defs"] = JSON {std::move(defs)};

    return body;
}

FunctionTool makeTool(std::string_view command,
                      std::string_view description,
                      TypeTree::TypeNode* request,
                      const ToolOptions& options)
{
    auto tool = FunctionTool {};
    tool.name = toolNameFor(command, options.nameSeparator);
    tool.description = std::string {description};
    tool.parameters = parametersFor(command, request);
    tool.strict = options.strict;
    return tool;
}

ToolSet ToolSet::fromDescription(Detail::DescribeReflector& describer,
                                 const ToolOptions& options)
{
    auto set = ToolSet {};

    for (auto& record: describer.commands)
    {
        auto* request =
            record.reqRoot ? &describer.typeRoots[*record.reqRoot] : nullptr;
        auto tool = makeTool(record.name, record.description, request, options);

        if (!set.commandNames.emplace(tool.name, record.name).second)
            throw std::invalid_argument(commandError(
                record.name,
                "tool name '" + tool.name + "' is already taken by command '"
                    + set.commandNames[tool.name] + "'"));

        set.functionTools.add(std::move(tool));
    }

    return set;
}

JSON ToolSet::definitions() const
{
    auto array = Json::Array {};

    for (auto& tool: functionTools)
        array.add(toJSON(tool));

    return JSON {std::move(array)};
}

const std::string* ToolSet::commandFor(std::string_view toolName) const
{
    auto found = commandNames.find(toolName);
    return found == commandNames.end() ? nullptr : &found->second;
}

FunctionCallOutput ToolSet::invoke(const Bridge& bridge,
                                   const FunctionCall& call) const
{
    auto result = FunctionCallOutput {};
    result.callId = call.callId;

    auto* command = commandFor(call.name);

    if (command == nullptr)
    {
        result.output = errorOutput("unknown function '" + call.name + "'");
        return result;
    }

    auto arguments = JSON {};

    try
    {
        arguments = Json::parse(call.arguments);
    }
    catch (const std::exception& e)
    {
        result.output = errorOutput(std::string {"invalid arguments: "} + e.what());
        return result;
    }

    try
    {
        result.output = Json::print(bridge.dispatch(*command, arguments));
    }
    catch (const std::exception& e)
    {
        result.output = errorOutput(e.what());
    }

    return result;
}

void ToolSet::invokeAsync(const Bridge& bridge,
                          const FunctionCall& call,
                          const std::function<void(FunctionCallOutput)>& done) const
{
    auto result = FunctionCallOutput {};
    result.callId = call.callId;

    auto* command = commandFor(call.name);

    if (command == nullptr)
    {
        result.output = errorOutput("unknown function '" + call.name + "'");
        done(std::move(result));
        return;
    }

    auto arguments = JSON {};

    try
    {
        arguments = Json::parse(call.arguments);
    }
    catch (const std::exception& e)
    {
        result.output = errorOutput(std::string {"invalid arguments: "} + e.what());
        done(std::move(result));
        return;
    }

    auto settle =
        [callId = call.callId, done](const JSON& value, const std::string* error)
    {
        auto output = FunctionCallOutput {};
        output.callId = callId;
        output.output = error != nullptr ? errorOutput(*error) : Json::print(value);
        done(std::move(output));
    };

    try
    {
        bridge.dispatchAsync(*command, arguments, settle);
    }
    catch (const std::exception& e)
    {
        auto message = std::string {e.what()};
        settle(JSON {}, &message);
    }
}

} // namespace Miro::OpenAI
