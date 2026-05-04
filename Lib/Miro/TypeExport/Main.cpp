// Pre-built main() for the Miro type-export runner. Linked into the
// executable target created by miro_add_type_export() in CMake. User
// code provides MIRO_EXPORT_TYPES(...) registrations; this file walks the
// registry once per requested format, bundles every registered type's
// reflected output into a single module, and writes it as one file per
// format. The registry stays format-agnostic — adding a new format is
// a one-line addition to the kFormats table below.

#include "../CommandExport/Register.h"
#include "../Cpp/Cpp.h"
#include "../JSON/Json.h"
#include "../Schema/Schema.h"
#include "../TypeScript/TypeScript.h"
#include "../TypeTree/TypeTree.h"
#include "Register.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

using Miro::TypeExport::TypeEntry;

using EntryPtr = std::unique_ptr<TypeEntry>;
using EntryList = std::vector<EntryPtr>;

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
// move-only (own unique_ptrs internally), so we reserve and emplace.
std::vector<Miro::TypeTree::TypeNode> buildAllTypeTrees(const EntryList& entries)
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
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

// Resolves a final TypeScript-level type name (post collision rewrite)
// for each named type, plus a flag for "object with zero fields" so the
// backend generator can elide empty request parameters. Keyed by the
// raw qualified C++ name, which matches CommandEntry::*QualifiedName.
struct ResolvedTypes
{
    std::map<std::string, std::string> finalNameByQualified;
    std::map<std::string, bool> emptyByQualified;
};

ResolvedTypes resolveTypes(const EntryList& entries)
{
    auto roots = buildAllTypeTrees(entries);
    auto rootSpan = std::span<Miro::TypeTree::TypeNode> {roots};
    Miro::TypeTree::prepareRoots(rootSpan); // rewrites typeName in place

    auto resolved = ResolvedTypes {};
    for (auto& root: roots)
    {
        if (root.qualifiedName.empty())
            continue;

        resolved.finalNameByQualified[root.qualifiedName] = root.typeName;
        resolved.emptyByQualified[root.qualifiedName] =
            root.shape == Miro::TypeTree::TypeNode::Shape::Object
            && root.fields.empty();
    }
    return resolved;
}

// One node in the namespace tree built from CommandEntry::name. A node
// is either a leaf (carries a CommandEntry) or a branch (carries
// children). Mixing the two at the same path — e.g. registering both
// `api` and `api::ping` — is a structural error and is rejected by
// insertCommand.
struct CommandNode
{
    const Miro::CommandExport::CommandEntry* leaf = nullptr;

    // Insertion-ordered so the emitted JS tree mirrors registration
    // order rather than alphabetical, which matches the existing
    // single-level format.
    std::vector<std::pair<std::string, CommandNode>> children;
};

std::vector<std::string> splitOnDoubleColon(std::string_view name)
{
    auto out = std::vector<std::string> {};
    auto start = std::size_t {0};

    while (start <= name.size())
    {
        auto pos = name.find("::", start);
        auto segment = std::string {pos == std::string_view::npos
                                        ? name.substr(start)
                                        : name.substr(start, pos - start)};

        // Tolerate whitespace the preprocessor preserved from the
        // macro's stringification (e.g. MIRO_EXPORT_COMMAND( a :: b )).
        auto first = segment.find_first_not_of(" \t");
        auto last = segment.find_last_not_of(" \t");
        if (first != std::string::npos)
            segment = segment.substr(first, last - first + 1);
        else
            segment.clear();

        out.push_back(std::move(segment));

        if (pos == std::string_view::npos)
            break;
        start = pos + 2;
    }

    return out;
}

void insertCommand(CommandNode& root, const Miro::CommandExport::CommandEntry& cmd)
{
    auto path = splitOnDoubleColon(cmd.name);
    auto* node = &root;

    for (auto i = std::size_t {0}; i < path.size(); ++i)
    {
        auto& segment = path[i];
        auto isLast = (i + 1 == path.size());

        auto it = std::find_if(node->children.begin(),
                               node->children.end(),
                               [&](auto& c) { return c.first == segment; });

        if (it == node->children.end())
        {
            node->children.emplace_back(segment, CommandNode {});
            it = std::prev(node->children.end());
        }

        if (isLast)
        {
            if (it->second.leaf || !it->second.children.empty())
                throw std::runtime_error("command path collision at '" + cmd.name
                                         + "': segment '" + segment
                                         + "' is used as both a function and a "
                                           "namespace");
            it->second.leaf = &cmd;
        }
        else
        {
            if (it->second.leaf)
                throw std::runtime_error("command path collision at '" + cmd.name
                                         + "': segment '" + segment
                                         + "' is already a function");
            node = &it->second;
        }
    }
}

std::string indentString(int depth)
{
    // Braced init-list would bind to initializer_list<char>, not the
    // (size_type, char) ctor we want.
    // NOLINTNEXTLINE(modernize-return-braced-init-list)
    return std::string(static_cast<std::size_t>(depth) * 4, ' ');
}

void emitLeaf(std::ostringstream& out,
              int depth,
              const Miro::CommandExport::CommandEntry& cmd,
              const ResolvedTypes& resolved)
{
    auto resName = resolved.finalNameByQualified.count(cmd.responseQualifiedName)
                       ? resolved.finalNameByQualified.at(cmd.responseQualifiedName)
                       : cmd.responseTypeName;

    auto reqEmpty = resolved.emptyByQualified.count(cmd.requestQualifiedName)
                    && resolved.emptyByQualified.at(cmd.requestQualifiedName);

    auto bodyIndent = indentString(depth + 1);

    if (reqEmpty)
    {
        out << "(): Promise<T." << resName << "> =>\n"
            << bodyIndent << "invoke('" << cmd.name << "', {}) as Promise<T."
            << resName << ">";
    }
    else
    {
        auto reqName =
            resolved.finalNameByQualified.count(cmd.requestQualifiedName)
                ? resolved.finalNameByQualified.at(cmd.requestQualifiedName)
                : cmd.requestTypeName;

        out << "(req: T." << reqName << "): Promise<T." << resName << "> =>\n"
            << bodyIndent << "invoke('" << cmd.name << "', req) as Promise<T."
            << resName << ">";
    }
}

void emitNode(std::ostringstream& out,
              int depth,
              const CommandNode& node,
              const ResolvedTypes& resolved)
{
    out << "{\n";

    auto childIndent = indentString(depth + 1);
    auto closeIndent = indentString(depth);

    for (auto& [name, child]: node.children)
    {
        out << childIndent << name << ": ";

        if (child.leaf != nullptr)
            emitLeaf(out, depth + 1, *child.leaf, resolved);
        else
            emitNode(out, depth + 1, child, resolved);

        out << ",\n";
    }

    out << closeIndent << "}";
}

std::string formatBackendModule(const EntryList& typeEntries,
                                std::string_view baseName)
{
    auto resolved = resolveTypes(typeEntries);

    auto root = CommandNode {};
    for (auto& cmd: Miro::CommandExport::Detail::registry())
        insertCommand(root, cmd);

    auto out = std::ostringstream {};
    out << "import type * as T from './" << baseName << "';\n\n";
    out << "export type Invoke = (command: string, payload: unknown) => "
           "Promise<unknown>;\n\n";
    out << "export function makeBackend(invoke: Invoke)\n{\n";
    out << "    return ";
    emitNode(out, 1, root, resolved);
    out << ";\n}\n";

    return out.str();
}

// Add new formats here. The runner doesn't care what each format does —
// it just calls generate(entries, baseName) and writes the result to
// <baseName><ext>.
const auto kFormats = std::vector<Format> {
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
            return Miro::Cpp::formatHeader(trees, /*withMiro=*/false);
        },
    },
    Format {
        "cpp-miro",
        ".miro.h",
        [](const EntryList& entries, std::string_view)
        {
            auto trees = buildAllTypeTrees(entries);
            return Miro::Cpp::formatHeader(trees, /*withMiro=*/true);
        },
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
        writeFile(outDir / fileName, fmt.generate(entries, baseName));
    }

    return 0;
}
