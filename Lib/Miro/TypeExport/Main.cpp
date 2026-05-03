// Pre-built main() for the Miro type-export runner. Linked into the
// executable target created by miro_add_type_export() in CMake. User
// code provides MIRO_EXPORT_TYPES(...) registrations; this file walks the
// registry once per requested format, bundles every registered type's
// reflected output into a single module, and writes it as one file per
// format. The registry stays format-agnostic — adding a new format is
// a one-line addition to the kFormats table below.

#include "../TypeScript/TypeScript.h"
#include "Register.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace Miro::TypeExport::Detail
{

std::vector<std::unique_ptr<TypeEntry>>& registry()
{
    static auto entries = std::vector<std::unique_ptr<TypeEntry>> {};
    return entries;
}

} // namespace Miro::TypeExport::Detail

namespace
{

using Miro::TypeExport::TypeEntry;

using EntryPtr = std::unique_ptr<TypeEntry>;
using EntryList = std::vector<EntryPtr>;

struct Format
{
    std::string_view name;
    std::string_view extension;
    std::function<std::string(const EntryList&)> generate;
};

// Reflects every entry into its own TsNode tree. The trees are
// move-only (own unique_ptrs internally), so we reserve and emplace.
std::vector<Miro::TypeScript::Detail::TsNode>
    buildAllTsTrees(const EntryList& entries)
{
    auto roots = std::vector<Miro::TypeScript::Detail::TsNode> {};
    roots.reserve(entries.size());

    for (auto& entry: entries)
    {
        auto opts = entry->topLevelOptions(Miro::Mode::Save, /*schema=*/true);
        auto& root = roots.emplace_back();
        auto refl = Miro::TypeScript::TypeScriptReflector {root, opts};
        entry->reflect(refl);
    }

    return roots;
}

// Add new formats here. The runner doesn't care what each format does —
// it just calls generate(entries) and writes the result to <name><ext>.
const auto kFormats = std::vector<Format> {
    Format {
        "zod",
        ".zod.ts",
        [](const EntryList& entries)
        { return Miro::TypeScript::formatZodModule(buildAllTsTrees(entries)); },
    },
    Format {
        "ts",
        ".ts",
        [](const EntryList& entries)
        { return Miro::TypeScript::formatTypesModule(buildAllTsTrees(entries)); },
    },
};

void writeFile(const std::filesystem::path& path, const std::string& contents)
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

} // namespace

int main(int argc, char** argv)
{
    auto outDir = std::filesystem::path {};
    auto baseName = std::string {"schema"};
    auto requestedFormats = std::set<std::string> {};

    for (auto i = 1; i < argc; ++i)
    {
        auto arg = std::string_view {argv[i]};

        if (arg == "--out" && i + 1 < argc)
            outDir = argv[++i];
        else if (arg == "--name" && i + 1 < argc)
            baseName = argv[++i];
        else if (arg == "--format" && i + 1 < argc)
            requestedFormats.insert(argv[++i]);
        else
        {
            usage(argv[0]);
            return 1;
        }
    }

    if (outDir.empty())
    {
        usage(argv[0]);
        return 1;
    }

    std::filesystem::create_directories(outDir);

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
        if (!requestedFormats.empty()
            && !requestedFormats.contains(std::string {fmt.name}))
            continue;

        auto fileName = baseName + std::string {fmt.extension};
        writeFile(outDir / fileName, fmt.generate(entries));
    }

    return 0;
}
