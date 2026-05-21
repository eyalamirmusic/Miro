// Built-in format registrations for the Miro codegen runner.
//
// Each registerFormat() call lands in a [[maybe_unused]] constant whose
// initialization runs at program startup, populating the format
// registry that main() walks. Downstream libraries plug in additional
// formats from their own TUs using the same Format.h API.
//
// This TU lives in the MiroTypeExportMain OBJECT library (alongside
// Main.cpp), not in the Miro static library — OBJECT-library TUs are
// always linked into the final executable in full, so the static-init
// registrations are guaranteed to run without needing a force-link
// anchor.

#include "Format.h"

#include "../CommandExport/CommandExport.h"
#include "../CommandExport/Register.h"
#include "../Cpp/Cpp.h"
#include "../Cpp/CppClient.h"
#include "../JSON/Json.h"
#include "../Schema/Schema.h"
#include "../TypeScript/TypeScript.h"

namespace
{

using Miro::TypeExport::EntryList;
using Miro::TypeExport::Format;
using Miro::TypeExport::registerFormat;

[[maybe_unused]] const auto zodFormat = registerFormat(Format {
    "zod",
    ".zod.ts",
    [](const EntryList& entries, std::string_view)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::TypeScript::formatZodModule(trees);
    },
});

[[maybe_unused]] const auto tsFormat = registerFormat(Format {
    "ts",
    ".ts",
    [](const EntryList& entries, std::string_view)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::TypeScript::formatTypesModule(trees);
    },
});

[[maybe_unused]] const auto backendFormat = registerFormat(Format {
    "backend",
    ".backend.ts",
    [](const EntryList& entries, std::string_view baseName)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::CommandExport::formatBackendModule(
            std::span<Miro::TypeTree::TypeNode> {trees},
            Miro::CommandExport::Detail::registry(),
            baseName);
    },
});

[[maybe_unused]] const auto tsServerFormat = registerFormat(Format {
    "ts-server",
    ".handlers.ts",
    [](const EntryList& entries, std::string_view baseName)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::CommandExport::formatServerHandlersModule(
            std::span<Miro::TypeTree::TypeNode> {trees},
            Miro::CommandExport::Detail::registry(),
            baseName);
    },
});

[[maybe_unused]] const auto bridgeFormat = registerFormat(Format {
    "bridge",
    ".bridge.ts",
    [](const EntryList&, std::string_view)
    { return Miro::TypeScript::formatBridgeRuntime(); },
});

[[maybe_unused]] const auto jsonSchemaFormat = registerFormat(Format {
    "jsonschema",
    ".schema.json",
    [](const EntryList& entries, std::string_view)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        auto schema = Miro::formatJsonSchema(trees);
        return Miro::Json::print(schema, 2);
    },
});

[[maybe_unused]] const auto cppFormat = registerFormat(Format {
    "cpp",
    ".types.h",
    [](const EntryList& entries, std::string_view)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::Cpp::formatHeader(trees, Miro::Cpp::Modes::PureCPP);
    },
});

[[maybe_unused]] const auto cppMiroFormat = registerFormat(Format {
    "cpp-miro",
    ".miro.h",
    [](const EntryList& entries, std::string_view)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::Cpp::formatHeader(trees, Miro::Cpp::Modes::Miro);
    },
});

[[maybe_unused]] const auto cppClientFormat = registerFormat(Format {
    "cpp-client",
    ".client.h",
    [](const EntryList& entries, std::string_view baseName)
    {
        auto trees = Miro::TypeExport::buildAllTypeTrees(entries);
        return Miro::Cpp::formatClientHeader(
            std::span<Miro::TypeTree::TypeNode> {trees},
            Miro::CommandExport::Detail::registry(),
            baseName);
    },
});

} // namespace
