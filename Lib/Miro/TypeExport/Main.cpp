// Pre-built main() for the Miro type-export runner. Linked into the
// executable target created by miro_export() in CMake. User
// code provides MIRO_EXPORT_TYPES(...) registrations; this file walks the
// registry once per requested format, bundles every registered type's
// reflected output into a single module, and writes it as one file per
// format. The registry stays format-agnostic — adding a new format is
// a one-line addition to the kFormats table below.

#include "../CommandExport/CommandExport.h"
#include "../CommandExport/Register.h"
#include "../Cpp/Cpp.h"
#include "../Cpp/CppClient.h"
#include "../JSON/Json.h"
#include "../Schema/Schema.h"
#include "../TypeScript/TypeScript.h"
#include "../TypeTree/TypeTree.h"
#include "Register.h"

#include <ghc/filesystem.hpp>

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

using Miro::TypeExport::TypeEntry;

using EntryList = Miro::OwnedVector<TypeEntry>;

struct Format
{
    std::string_view name;
    std::string_view extension;
    // baseName is the output filename stem (e.g. "schema"). Formats that
    // emit a self-contained module ignore it; the backend wrapper uses it
    // to import the matching types module by relative path.
    std::function<std::string(const EntryList&, std::string_view baseName)> generate;
};

// Reflects every entry into its own TypeNode tree. The trees are
// move-only (own OwningPointers internally), so we reserve and emplace.
Miro::Vector<Miro::TypeTree::TypeNode> buildAllTypeTrees(const EntryList& entries)
{
    auto roots = Miro::Vector<Miro::TypeTree::TypeNode> {};
    roots.reserve(entries.size());

    for (auto& entry: entries)
    {
        auto opts = entry->topLevelOptions(Miro::Mode::Save, /*schema=*/true);
        auto& root = roots.emplace_back();
        auto refl = Miro::TypeTree::TypeReflector {root, opts};
        entry->reflect(refl);
    }

    return roots;
}

std::string formatBackendModule(const EntryList& typeEntries,
                                std::string_view baseName)
{
    auto roots = buildAllTypeTrees(typeEntries);
    return Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots},
        Miro::CommandExport::Detail::registry(),
        baseName);
}

// Add new formats here. The runner doesn't care what each format does —
// it just calls generate(entries, baseName) and writes the result to
// <baseName><ext>.
const auto kFormats = Miro::Vector<Format> {
    Format {
        "zod",
        ".zod.ts",
        [](const EntryList& entries, std::string_view)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::TypeScript::formatZodModule(trees);
        },
    },
    Format {
        "ts",
        ".ts",
        [](const EntryList& entries, std::string_view)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::TypeScript::formatTypesModule(trees);
        },
    },
    Format {
        "backend",
        ".backend.ts",
        [](const EntryList& entries, std::string_view baseName)
        { return formatBackendModule(entries, baseName); },
    },
    Format {
        "ts-server",
        ".handlers.ts",
        [](const EntryList& entries, std::string_view baseName)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::CommandExport::formatServerHandlersModule(
                std::span<Miro::TypeTree::TypeNode> {trees},
                Miro::CommandExport::Detail::registry(),
                baseName);
        },
    },
    Format {
        "bridge",
        ".bridge.ts",
        [](const EntryList&, std::string_view)
        { return Miro::TypeScript::formatBridgeRuntime(); },
    },
    Format {
        "jsonschema",
        ".schema.json",
        [](const EntryList& entries, std::string_view)
        {
            auto trees = buildAllTypeTrees(entries);
            auto schema = Miro::formatJsonSchema(trees);
            return Miro::Json::print(schema, 2);
        },
    },
    Format {
        "cpp",
        ".types.h",
        [](const EntryList& entries, std::string_view)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::Cpp::formatHeader(trees, Miro::Cpp::Modes::PureCPP);
        },
    },
    Format {
        "cpp-miro",
        ".miro.h",
        [](const EntryList& entries, std::string_view)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::Cpp::formatHeader(trees, Miro::Cpp::Modes::Miro);
        },
    },
    Format {
        "cpp-client",
        ".client.h",
        [](const EntryList& entries, std::string_view baseName)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::Cpp::formatClientHeader(
                std::span<Miro::TypeTree::TypeNode> {trees},
                Miro::CommandExport::Detail::registry(),
                baseName);
        },
    },
};

void writeFile(const ghc::filesystem::path& path, const std::string& contents)
{
    auto out = std::ofstream {path};
    out << contents;
    std::cout << "Wrote " << path.string() << "\n";
}

void usage(const char* exeName)
{
    std::cerr << "Usage: " << exeName
              << " --out <dir> [--name <basename>] [--format <name>]...\n"
              << "  --out <dir>       Directory to write generated files into\n"
              << "  --name <basename> Output filename stem (default: schema)\n"
              << "  --format <name>   Repeatable; defaults to all known formats\n"
              << "Known formats:";
    for (auto& fmt: kFormats)
        std::cerr << " " << fmt.name;
    std::cerr << "\n";
}

struct Args
{
    ghc::filesystem::path outDir;
    std::string baseName = "schema";
    Miro::Vector<std::string> requestedFormats;
    bool valid = false;
};

Args parseArgs(int argc, char** argv)
{
    auto args = Args {};

    for (auto i = 1; i < argc; ++i)
    {
        auto arg = std::string_view {argv[i]};

        if (arg == "--out" && i + 1 < argc)
            args.outDir = argv[++i];
        else if (arg == "--name" && i + 1 < argc)
            args.baseName = argv[++i];
        else if (arg == "--format" && i + 1 < argc)
            args.requestedFormats.addIfNotThere(argv[++i]);
        else
            return args;
    }

    args.valid = !args.outDir.empty();
    return args;
}

bool isFormatRequested(const Miro::Vector<std::string>& requested,
                       std::string_view formatName)
{
    return requested.empty() || requested.contains(std::string {formatName});
}

} // namespace

int main(int argc, char** argv)
{
    auto args = parseArgs(argc, argv);

    if (!args.valid)
    {
        usage(argv[0]);
        return 1;
    }

    ghc::filesystem::create_directories(args.outDir);

    auto& entries = Miro::TypeExport::Detail::registry();

    if (entries.empty())
    {
        std::cerr << "No types registered. The registration library must be "
                     "linked with WHOLE_ARCHIVE so its static initializers "
                     "survive linking.\n";
        return 1;
    }

    for (auto& fmt: kFormats)
    {
        if (!isFormatRequested(args.requestedFormats, fmt.name))
            continue;

        auto fileName = args.baseName + std::string {fmt.extension};
        writeFile(args.outDir / fileName, fmt.generate(entries, args.baseName));
    }

    return 0;
}
