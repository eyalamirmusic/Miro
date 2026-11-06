#pragma once

/** @file
    OpenAI Responses-API function tools over a reflected API. A ToolSet
    walks the API's reflect() body the way codegen does, turns each
    command into a "function" tool definition (its request type rendered
    as a strict JSON Schema), and routes the model's "function_call"
    items back through a Bridge:

      auto tools = Miro::OpenAI::ToolSet::describe(api);
      request["tools"] = tools.definitions();
      ...
      auto call = Miro::createFromJSON<Miro::OpenAI::FunctionCall>(item);
      nextInput.push_back(Miro::toJSON(tools.invoke(bridge, call)));

    Wire keys follow OpenAI's spelling (call_id); C++ members are
    camelCase.
*/

#include "../Bridge/Bridge.h"
#include "../Bridge/DescribeReflector.h"
#include "../JSON/Json.h"
#include "../Reflection/Omittable.h"
#include "../Reflection/Reflector.h"
#include "../TypeTree/TypeTree.h"

#include <ea_data_structures/Structures/Vector.h>

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace Miro::OpenAI
{

/// One "function_call" item from a response's output array.
struct FunctionCall
{
    void reflect(Reflector& ref);

    std::string type {"function_call"};
    Omittable<std::string> id;
    std::string callId;
    std::string name;

    /// The arguments object, JSON-encoded as a string.
    std::string arguments;
    Omittable<std::string> status;
};

/// The "function_call_output" item answering a FunctionCall.
struct FunctionCallOutput
{
    void reflect(Reflector& ref);

    std::string type {"function_call_output"};
    std::string callId;

    /// The command's result (or {"error": ...}), JSON-encoded.
    std::string output;
};

/// One entry of a request's "tools" array.
struct FunctionTool
{
    void reflect(Reflector& ref);

    std::string type {"function"};
    std::string name;
    std::string description;
    JSON parameters;
    bool strict {true};
};

/// Options for building tools from an API.
struct ToolOptions
{
    /// Emitted as each tool's "strict" flag. The parameters schema is
    /// always the strict dialect, which non-strict mode accepts too.
    bool strict {true};

    /// Replaces the '.' of a sub-API's dotted command name, which
    /// OpenAI's [A-Za-z0-9_-]{1,64} tool names can't hold.
    std::string_view nameSeparator {"__"};
};

/// The tool name for a command: '.' becomes the separator, and anything
/// that still isn't a legal tool name throws std::invalid_argument.
std::string toolNameFor(std::string_view command, std::string_view separator = "__");

/// A command's "parameters" schema: the request type's own definition
/// inlined at the root (other named types stay under "$defs" beside
/// it), or an empty closed object when the command takes no request.
/// Throws std::invalid_argument, prefixed with the command name, when
/// the request isn't an object (a bare tagged union) or holds something
/// the strict dialect can't express (a map, a raw JSON field).
JSON parametersFor(std::string_view command, TypeTree::TypeNode* request);

/// One tool from a command. Throws std::invalid_argument as toolNameFor and
/// parametersFor do; request may be nullptr for a command with no request.
FunctionTool makeTool(std::string_view command,
                      std::string_view description,
                      TypeTree::TypeNode* request,
                      const ToolOptions& options = {});

/// The tools for one API and the map from tool names back to commands.
/// Holds no reference to the API or to a Bridge.
class ToolSet
{
public:
    /// Walks api.reflect() with a DescribeReflector; nothing is bound or run.
    /// Throws std::invalid_argument when a command can't be a strict tool.
    template <typename Api>
    static ToolSet describe(Api& api, const ToolOptions& options = {})
    {
        auto describer = Detail::DescribeReflector {};
        api.reflect(describer);
        return fromDescription(describer, options);
    }

    /// Builds the tools from an already walked API. The trees are taken
    /// mutably because schema rendering may rename types.
    static ToolSet fromDescription(Detail::DescribeReflector& describer,
                                   const ToolOptions& options = {});

    const Vector<FunctionTool>& tools() const { return functionTools; }

    /// The "tools" array of a Responses-API request.
    JSON definitions() const;

    /// The command a tool name maps back to, nullptr when unknown.
    const std::string* commandFor(std::string_view toolName) const;

    /// Runs the call's command on the bridge. Never throws: an unknown
    /// tool, malformed arguments or a throwing handler all come back as
    /// an {"error": "..."} output for the model to read.
    FunctionCallOutput invoke(const Bridge& bridge, const FunctionCall& call) const;

    /// Same through Bridge::dispatchAsync, so async commands work too.
    /// `done` runs once, possibly later and on another thread.
    void invokeAsync(const Bridge& bridge,
                     const FunctionCall& call,
                     const std::function<void(FunctionCallOutput)>& done) const;

private:
    Vector<FunctionTool> functionTools;
    std::map<std::string, std::string, std::less<>> commandNames;
};

} // namespace Miro::OpenAI
