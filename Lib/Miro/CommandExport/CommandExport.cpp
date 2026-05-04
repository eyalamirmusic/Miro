#include "CommandExport.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Miro::CommandExport
{

namespace
{

// One node in the namespace tree built from CommandEntry::name. A node
// is either a leaf (carries a CommandEntry) or a branch (carries
// children). Mixing the two at the same path — e.g. registering both
// `api` and `api::ping` — is a structural error and is rejected by
// insertCommand.
struct CommandNode
{
    const CommandEntry* leaf = nullptr;

    // Insertion-ordered so the emitted JS tree mirrors registration
    // order rather than alphabetical.
    std::vector<std::pair<std::string, CommandNode>> children;
};

// Resolved per-type info derived from the supplied TypeNode roots:
// the final TypeScript-level name (post collision rewrite), and a
// flag for "object with zero fields" so empty-request handlers can
// elide their parameter. Keyed by raw qualified C++ name, which
// matches CommandEntry::*QualifiedName.
struct ResolvedTypes
{
    std::map<std::string, std::string> finalNameByQualified;
    std::map<std::string, bool> emptyByQualified;
};

ResolvedTypes resolveTypes(std::span<TypeTree::TypeNode> typeRoots)
{
    TypeTree::prepareRoots(typeRoots);

    auto resolved = ResolvedTypes {};
    for (auto& root: typeRoots)
    {
        if (root.qualifiedName.empty())
            continue;

        resolved.finalNameByQualified[root.qualifiedName] = root.typeName;
        resolved.emptyByQualified[root.qualifiedName] =
            root.shape == TypeTree::TypeNode::Shape::Object && root.fields.empty();
    }
    return resolved;
}

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

void insertCommand(CommandNode& root, const CommandEntry& cmd)
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
              const CommandEntry& cmd,
              const ResolvedTypes& resolved)
{
    auto resTypeStr = std::string {"void"};

    if (cmd.hasResponse)
    {
        auto resName =
            resolved.finalNameByQualified.count(cmd.responseQualifiedName)
                ? resolved.finalNameByQualified.at(cmd.responseQualifiedName)
                : cmd.responseTypeName;
        resTypeStr = "T." + resName;
    }

    // Param is elided either when the C++ handler takes none, or when
    // the request type is an empty struct (zero fields) — both produce
    // the same JS callsite shape.
    auto reqEmpty = cmd.hasRequest
                    && resolved.emptyByQualified.count(cmd.requestQualifiedName)
                    && resolved.emptyByQualified.at(cmd.requestQualifiedName);

    auto emitParam = cmd.hasRequest && !reqEmpty;

    auto bodyIndent = indentString(depth + 1);

    if (emitParam)
    {
        auto reqName =
            resolved.finalNameByQualified.count(cmd.requestQualifiedName)
                ? resolved.finalNameByQualified.at(cmd.requestQualifiedName)
                : cmd.requestTypeName;

        out << "(req: T." << reqName << "): Promise<" << resTypeStr << "> =>\n"
            << bodyIndent << "invoke('" << cmd.name << "', req) as Promise<"
            << resTypeStr << ">";
    }
    else
    {
        out << "(): Promise<" << resTypeStr << "> =>\n"
            << bodyIndent << "invoke('" << cmd.name << "', {}) as Promise<"
            << resTypeStr << ">";
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

} // namespace

std::string formatBackendModule(std::span<TypeTree::TypeNode> typeRoots,
                                std::span<const CommandEntry> commands,
                                std::string_view baseName)
{
    auto resolved = resolveTypes(typeRoots);

    auto root = CommandNode {};
    for (auto& cmd: commands)
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

} // namespace Miro::CommandExport
