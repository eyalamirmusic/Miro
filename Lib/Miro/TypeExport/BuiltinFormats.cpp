// Built-in format registrations for the Miro codegen runner.
//
// Each registerFormat() call lands in a [[maybe_unused]] constant whose
// initialization runs at program startup, populating the format
// registry that codegenMain<Apis...> walks. Downstream libraries plug
// in additional formats from their own TUs using the same Format.h API.
//
// This TU lives in the MiroFormats OBJECT library, not in the Miro
// static library — OBJECT-library TUs are always linked into the final
// executable in full, so the static-init format registrations are
// guaranteed to run without needing a force-link anchor.

#include "Format.h"

#include "../CommandExport/CommandExport.h"
#include "../Cpp/Cpp.h"
#include "../Cpp/CppClient.h"
#include "../JSON/Json.h"
#include "../OpenAI/OpenAI.h"
#include "../Reflection/Serialize.h"
#include "../Schema/Schema.h"
#include "../TypeScript/TypeScript.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace
{

using Miro::TypeExport::Context;
using Miro::TypeExport::Format;
using Miro::TypeExport::registerFormat;

[[maybe_unused]] const auto zodFormat = registerFormat(Format {
    "zod",
    ".zod.ts",
    [](const Context& ctx)
    { return Miro::TypeScript::formatZodModule(ctx.typeRoots); },
});

[[maybe_unused]] const auto tsFormat = registerFormat(Format {
    "ts",
    ".ts",
    [](const Context& ctx)
    { return Miro::TypeScript::formatTypesModule(ctx.typeRoots); },
});

[[maybe_unused]] const auto backendFormat = registerFormat(Format {
    "backend",
    ".backend.ts",
    [](const Context& ctx)
    {
        return Miro::CommandExport::formatBackendModule(
            ctx.typeRoots, ctx.commands, ctx.baseName);
    },
});

[[maybe_unused]] const auto tsServerFormat = registerFormat(Format {
    "ts-server",
    ".handlers.ts",
    [](const Context& ctx)
    {
        return Miro::CommandExport::formatServerHandlersModule(
            ctx.typeRoots, ctx.commands, ctx.baseName);
    },
});

[[maybe_unused]] const auto bridgeFormat = registerFormat(Format {
    "bridge",
    ".bridge.ts",
    [](const Context&) { return Miro::TypeScript::formatBridgeRuntime(); },
});

[[maybe_unused]] const auto eventsFormat = registerFormat(Format {
    "events",
    ".events.ts",
    [](const Context& ctx)
    {
        return Miro::TypeScript::formatEventsModule(
            ctx.typeRoots, ctx.events, ctx.baseName);
    },
});

[[maybe_unused]] const auto jsonSchemaFormat = registerFormat(Format {
    "jsonschema",
    ".schema.json",
    [](const Context& ctx)
    {
        auto schema = Miro::formatJsonSchema(ctx.typeRoots);
        return Miro::Json::print(schema, 2);
    },
});

// The same "tools" array OpenAI::ToolSet::definitions() returns. Context
// carries no command -> root index, so each request root is found by
// its qualified C++ name.
Miro::TypeTree::TypeNode* findRoot(const Context& ctx,
                                   const std::string& qualifiedName)
{
    for (auto& root: ctx.typeRoots)
        if (root.qualifiedName == qualifiedName)
            return &root;

    return nullptr;
}

[[maybe_unused]] const auto openAiToolsFormat = registerFormat(Format {
    "openai-tools",
    ".tools.json",
    [](const Context& ctx)
    {
        auto tools = Miro::Json::Array {};

        for (auto& command: ctx.commands)
        {
            auto* request = command.hasRequest
                                ? findRoot(ctx, command.requestQualifiedName)
                                : nullptr;

            if (command.hasRequest && request == nullptr)
                throw std::invalid_argument("openai-tools: no type tree for "
                                            + command.requestQualifiedName);

            tools.add(Miro::toJSON(
                Miro::OpenAI::makeTool(command.name, command.description, request)));
        }

        return Miro::Json::print(Miro::JSON {std::move(tools)}, 2);
    },
});

[[maybe_unused]] const auto cppFormat = registerFormat(Format {
    "cpp",
    ".types.h",
    [](const Context& ctx)
    { return Miro::Cpp::formatHeader(ctx.typeRoots, Miro::Cpp::Modes::PureCPP); },
});

[[maybe_unused]] const auto cppMiroFormat = registerFormat(Format {
    "cpp-miro",
    ".miro.h",
    [](const Context& ctx)
    { return Miro::Cpp::formatHeader(ctx.typeRoots, Miro::Cpp::Modes::Miro); },
});

[[maybe_unused]] const auto cppClientFormat = registerFormat(Format {
    "cpp-client",
    ".client.h",
    [](const Context& ctx)
    {
        return Miro::Cpp::formatClientHeader(
            ctx.typeRoots, ctx.commands, ctx.baseName);
    },
});

} // namespace
