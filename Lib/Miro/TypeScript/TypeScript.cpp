#include "TypeScript.h"

#include <concepts>
#include <set>
#include <sstream>
#include <type_traits>
#include <utility>

namespace Miro::TypeScript
{

using Detail::TsNode;

TypeScriptReflector::TypeScriptReflector(TsNode& nodeToUse, Options optsToUse)
    : Reflector(optsToUse)
    , node(nodeToUse)
{
    switch (opts.shape)
    {
        case Shape::Primitive:
            node.shape = TsNode::Shape::Primitive;
            break;
        case Shape::Object:
            node.shape = TsNode::Shape::Object;
            break;
        case Shape::Array:
            node.shape = TsNode::Shape::Array;
            node.inner = std::make_unique<TsNode>();
            break;
        case Shape::Map:
            node.shape = TsNode::Shape::Map;
            node.inner = std::make_unique<TsNode>();
            break;
    }

    node.optional = opts.nullable;
}

TypeScriptReflector::~TypeScriptReflector() = default;

ValueKind TypeScriptReflector::kind() const
{
    return ValueKind::Absent;
}

void TypeScriptReflector::writeNull() {}

void TypeScriptReflector::beginNamedType(std::string_view typeName)
{
    node.typeName = std::string {typeName};
}

void TypeScriptReflector::visit(PrimitiveRef ref)
{
    std::visit(
        [this](auto* ptr)
        {
            using T = std::remove_pointer_t<decltype(ptr)>;
            if constexpr (std::same_as<T, bool>)
                node.primitive = "z.boolean()";
            else if constexpr (std::same_as<T, std::string>)
                node.primitive = "z.string()";
            else if constexpr (std::same_as<T, double>)
                node.primitive = "z.number()";
            else
                node.primitive = "z.number().int()";
        },
        ref.data);
}

Reflector& TypeScriptReflector::spawnChild(TsNode& targetNode, Options childOpts)
{
    currentChild.reset();
    currentChild = std::make_unique<TypeScriptReflector>(targetNode, childOpts);
    return *currentChild;
}

Reflector& TypeScriptReflector::atKey(std::string_view key, Options childOpts)
{
    if (node.shape == TsNode::Shape::Map)
        return spawnChild(*node.inner, childOpts);

    auto field = TsNode::Field {std::string {key}, std::make_unique<TsNode>()};
    node.fields.push_back(std::move(field));
    return spawnChild(*node.fields.back().type, childOpts);
}

Reflector& TypeScriptReflector::atIndex(std::size_t, Options childOpts)
{
    return spawnChild(*node.inner, childOpts);
}

namespace
{

// Collects every named object node reachable from root in post-order
// (deepest first). Duplicates of the same name keep the first node we
// saw — re-encounters become refs.
void collectNamed(const TsNode& node,
                  std::set<std::string>& seen,
                  std::vector<const TsNode*>& outOrdered)
{
    if (node.shape == TsNode::Shape::Object && !node.typeName.empty())
    {
        if (seen.contains(node.typeName))
            return;

        seen.insert(node.typeName);

        for (auto& field: node.fields)
            collectNamed(*field.type, seen, outOrdered);

        outOrdered.push_back(&node);
        return;
    }

    if (node.shape == TsNode::Shape::Object)
    {
        for (auto& field: node.fields)
            collectNamed(*field.type, seen, outOrdered);
    }
    else if (node.inner)
    {
        collectNamed(*node.inner, seen, outOrdered);
    }
}

// Renders `node` as a Zod expression. Named object nodes always render
// as a bare identifier (the short name); the caller is responsible for
// having emitted that name's declaration earlier.
std::string render(const TsNode& node);

std::string renderObjectInline(const TsNode& node, std::string_view indent)
{
    auto out = std::ostringstream {};
    out << "z.object({\n";

    for (auto& field: node.fields)
    {
        out << indent << "    " << field.name << ": " << render(*field.type)
            << ",\n";
    }

    out << indent << "})";
    return out.str();
}

std::string render(const TsNode& node)
{
    auto base = std::string {};

    switch (node.shape)
    {
        case TsNode::Shape::Primitive:
            base = node.primitive.empty() ? "z.unknown()" : node.primitive;
            break;
        case TsNode::Shape::Object:
            base =
                node.typeName.empty() ? renderObjectInline(node, "") : node.typeName;
            break;
        case TsNode::Shape::Array:
            base = "z.array(" + render(*node.inner) + ")";
            break;
        case TsNode::Shape::Map:
            base = "z.record(z.string(), " + render(*node.inner) + ")";
            break;
    }

    if (node.optional)
        base += ".optional()";

    return base;
}

std::string declareNamed(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "export const " << node.typeName << " = " << renderObjectInline(node, "")
        << ";\n";
    out << "export type " << node.typeName << " = z.infer<typeof " << node.typeName
        << ">;\n";
    return out.str();
}

} // namespace

std::string formatModule(const TsNode& root)
{
    auto seen = std::set<std::string> {};
    auto ordered = std::vector<const TsNode*> {};
    collectNamed(root, seen, ordered);

    auto out = std::ostringstream {};
    out << "import { z } from \"zod\";\n\n";

    for (auto* node: ordered)
        out << declareNamed(*node) << "\n";

    // If the root is an anonymous shape (e.g. a top-level vector), emit
    // a default export for it so the module isn't pointless.
    if (root.shape != TsNode::Shape::Object || root.typeName.empty())
        out << "export default " << render(root) << ";\n";

    return out.str();
}

} // namespace Miro::TypeScript
