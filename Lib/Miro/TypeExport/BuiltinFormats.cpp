// Belongs to the MiroFormats OBJECT library, not the Miro static lib, so
// these static-init registrations are always linked into the executable.

#include "Format.h"

#include "../CommandExport/CommandExport.h"
#include "../Cpp/Cpp.h"
#include "../Cpp/CppClient.h"
#include "../JSON/Json.h"
#include "../Schema/Schema.h"
#include "../TypeScript/TypeScript.h"

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
