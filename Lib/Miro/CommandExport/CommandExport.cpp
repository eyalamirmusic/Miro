#include "CommandExport.h"

#include "../Detail/StringUtilities.h"
#include "ResolvedTypes.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Miro::CommandExport
{

namespace
{

// One node in the namespace tree built from CommandEntry::name. A node
// is either a leaf (carries a CommandEntry) or a branch (carries
// children). Mixing the two at the same path — e.g. registering both
// `api` and `api.ping` — is a structural error and is rejected by
// insertCommand.
//
// Children go through OwningPointer so CommandNode can hold a vector of
// itself without forcing the standard library to instantiate completeness
// traits on the still-incomplete CommandNode.
struct CommandNode;

struct CommandChild
{
    std::string name;
    OwningPointer<CommandNode> node;
};

struct CommandNode
{
    const CommandEntry* leaf = nullptr;

    // Insertion-ordered so the emitted JS tree mirrors registration
    // order rather than alphabetical.
    Vector<CommandChild> children;
};

Vector<std::string> commandPathSegments(std::string_view name)
{
    // ApiReflector::joinedName joins sub-API prefixes with a literal '.'
    // (the wire-protocol separator the Bridge dispatches against), so
    // r.use("todos", sub) yielding a method `getChanged` lands here as
    // "todos.getChanged". Split on that same '.' so the emitted module
    // mirrors the recursion shape: { todos: { getChanged: ... } }.
    auto segments = Miro::Detail::splitOn(name, ".");

    // Tolerate whitespace around the separator so a command name like
    // "a . b" (whether handwritten or produced by a name-derivation
    // path that preserves it) still splits cleanly into ["a", "b"].
    for (auto& segment: segments)
        segment = Miro::Detail::trimAsciiWhitespace(segment);

    return segments;
}

CommandChild& findOrCreateChild(CommandNode& node, const std::string& segment)
{
    auto it = std::ranges::find_if(node.children,
                                   [&](auto& c) { return c.name == segment; });

    if (it == node.children.end())
        return node.children.create(segment, EA::makeOwned<CommandNode>());

    return *it;
}

[[noreturn]] void throwPathCollision(const std::string& cmdName,
                                     const std::string& segment,
                                     std::string_view reason)
{
    throw std::runtime_error("command path collision at '" + cmdName + "': segment '"
                             + segment + "' " + std::string {reason});
}

void assignLeaf(CommandNode& node,
                const std::string& segment,
                const CommandEntry& cmd)
{
    if (node.leaf || !node.children.empty())
        throwPathCollision(
            cmd.name, segment, "is used as both a function and a namespace");

    node.leaf = &cmd;
}

void insertCommand(CommandNode& root, const CommandEntry& cmd)
{
    auto path = commandPathSegments(cmd.name);
    auto* node = &root;

    for (auto i = 0; i < path.size(); ++i)
    {
        auto& segment = path[i];
        auto isLast = (i + 1 == path.size());

        auto& child = findOrCreateChild(*node, segment);
        auto& childNode = *child.node;

        if (isLast)
        {
            assignLeaf(childNode, segment, cmd);
            continue;
        }

        if (childNode.leaf)
            throwPathCollision(cmd.name, segment, "is already a function");

        node = &childNode;
    }
}

std::string indentString(int depth)
{
    return Miro::Detail::makeIndent(4, depth);
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
            resolved.nameFor(cmd.responseQualifiedName, cmd.responseTypeName);
        resTypeStr = "T." + resName;
    }

    auto emitParam = !resolved.isRequestEmpty(cmd);
    auto bodyIndent = indentString(depth + 1);

    if (emitParam)
    {
        auto reqName =
            resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);

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

    for (auto& c: node.children)
    {
        out << childIndent << c.name << ": ";

        if (c.node->leaf != nullptr)
            emitLeaf(out, depth + 1, *c.node->leaf, resolved);
        else
            emitNode(out, depth + 1, *c.node, resolved);

        out << ",\n";
    }

    out << closeIndent << "}";
}

std::string handlerReturnType(const CommandEntry& cmd, const ResolvedTypes& resolved)
{
    if (!cmd.hasResponse)
        return "void | Promise<void>";

    auto resName =
        "T." + resolved.nameFor(cmd.responseQualifiedName, cmd.responseTypeName);
    return resName + " | Promise<" + resName + ">";
}

void emitHandlerSignature(std::ostringstream& out,
                          const CommandEntry& cmd,
                          const ResolvedTypes& resolved)
{
    auto emitParam = !resolved.isRequestEmpty(cmd);

    out << "(";

    if (emitParam)
    {
        auto reqName =
            resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);
        out << "req: T." << reqName;
    }

    out << "): " << handlerReturnType(cmd, resolved);
}

void emitHandlerType(std::ostringstream& out,
                     int depth,
                     const CommandNode& node,
                     const ResolvedTypes& resolved)
{
    out << "{\n";

    auto childIndent = indentString(depth + 1);
    auto closeIndent = indentString(depth);

    for (auto& c: node.children)
    {
        out << childIndent << c.name;

        if (c.node->leaf != nullptr)
            emitHandlerSignature(out, *c.node->leaf, resolved);
        else
        {
            out << ": ";
            emitHandlerType(out, depth + 1, *c.node, resolved);
        }

        out << ";\n";
    }

    out << closeIndent << "}";
}

void emitDispatchCases(std::ostringstream& out,
                       const CommandNode& node,
                       const ResolvedTypes& resolved,
                       const std::string& accessPrefix)
{
    for (auto& c: node.children)
    {
        auto access = accessPrefix.empty() ? c.name : accessPrefix + "." + c.name;

        if (c.node->leaf == nullptr)
        {
            emitDispatchCases(out, *c.node, resolved, access);
            continue;
        }

        auto& cmd = *c.node->leaf;
        auto emitParam = !resolved.isRequestEmpty(cmd);

        out << "        case '" << cmd.name << "': return await handlers." << access
            << "(";

        if (emitParam)
        {
            auto reqName =
                resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);
            out << "payload as T." << reqName;
        }

        out << ");\n";
    }
}

CommandNode buildCommandTree(std::span<const CommandEntry> commands)
{
    auto root = CommandNode {};

    for (auto& cmd: commands)
        insertCommand(root, cmd);

    return root;
}

} // namespace

std::string formatBackendModule(std::span<TypeTree::TypeNode> typeRoots,
                                std::span<const CommandEntry> commands,
                                std::string_view baseName)
{
    auto resolved = resolveTypes(typeRoots);
    auto root = buildCommandTree(commands);

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

std::string formatServerHandlersModule(std::span<TypeTree::TypeNode> typeRoots,
                                       std::span<const CommandEntry> commands,
                                       std::string_view baseName)
{
    auto resolved = resolveTypes(typeRoots);
    auto root = buildCommandTree(commands);

    auto out = std::ostringstream {};
    out << "import type * as T from './" << baseName << "';\n\n";

    out << "export type Handlers = ";
    emitHandlerType(out, 0, root, resolved);
    out << ";\n\n";

    out << "export class UnknownCommandError extends Error\n"
           "{\n"
           "    httpStatus = 404;\n"
           "    constructor(command: string)\n"
           "    {\n"
           "        super(`Unknown command: ${command}`);\n"
           "    }\n"
           "}\n\n";

    auto anyCommandUsesPayload = std::ranges::any_of(
        commands, [&](auto& cmd) { return !resolved.isRequestEmpty(cmd); });
    auto payloadParam = anyCommandUsesPayload ? "payload" : "_payload";

    out << "export async function dispatch(handlers: Handlers, "
           "command: string, "
        << payloadParam
        << ": unknown): Promise<unknown>\n"
           "{\n"
           "    switch (command)\n"
           "    {\n";
    emitDispatchCases(out, root, resolved, "");
    out << "        default: throw new UnknownCommandError(command);\n"
           "    }\n"
           "}\n";

    return out.str();
}

} // namespace Miro::CommandExport
