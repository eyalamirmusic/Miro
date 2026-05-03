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

void TypeScriptReflector::beginNamedType(TypeId id)
{
    node.typeName = std::string {id.shortName};
    node.qualifiedName = std::string {id.qualifiedName};
}

void TypeScriptReflector::visitEnum(TypeId id,
                                    const std::vector<std::string_view>& names)
{
    node.shape = TsNode::Shape::Enum;
    node.typeName = std::string {id.shortName};
    node.qualifiedName = std::string {id.qualifiedName};
    node.enumValues.clear();
    node.enumValues.reserve(names.size());

    for (auto& name: names)
        node.enumValues.emplace_back(name);
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

// True if `name` matches /^[A-Za-z_$][A-Za-z0-9_$]*$/ — the ASCII subset
// of valid JavaScript identifiers, which is enough to decide whether an
// object/interface key can be emitted bare or needs to be quoted.
bool isJsIdentifier(std::string_view name)
{
    if (name.empty())
        return false;

    auto isStart = [](char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'
               || c == '$';
    };

    auto isPart = [&](char c) { return isStart(c) || (c >= '0' && c <= '9'); };

    if (!isStart(name.front()))
        return false;

    for (auto c: name.substr(1))
        if (!isPart(c))
            return false;

    return true;
}

// Returns `name` ready to drop into a JS object literal or TS interface
// as a property key. Bare identifier when possible; otherwise a JSON-
// quoted string with `\` and `"` escaped. `MIRO_REFLECT_MEMBERS` lets
// users pick arbitrary keys, so we have to defend against spaces and
// other non-identifier characters.
std::string formatPropertyKey(std::string_view name)
{
    if (isJsIdentifier(name))
        return std::string {name};

    auto out = std::string {"\""};
    for (auto c: name)
    {
        if (c == '\\' || c == '"')
            out += '\\';
        out += c;
    }
    out += '"';
    return out;
}

// Identity used to dedup a node in collectNamed. Prefers the raw
// qualified name (always unique per C++ type); falls back to the short
// name for the rare anonymous-object case so an unnamed slot still
// pairs with itself if encountered twice.
const std::string& dedupKey(const TsNode& node)
{
    return node.qualifiedName.empty() ? node.typeName : node.qualifiedName;
}

// Collects every named object/enum node reachable from root in
// post-order (deepest first). Re-encounters of the same C++ type (by
// qualified name) become inline name references in the rendered output.
void collectNamed(const TsNode& node,
                  std::set<std::string>& seen,
                  std::vector<const TsNode*>& outOrdered)
{
    if (node.shape == TsNode::Shape::Object && !node.typeName.empty())
    {
        auto& key = dedupKey(node);
        if (seen.contains(key))
            return;

        seen.insert(key);

        for (auto& field: node.fields)
            collectNamed(*field.type, seen, outOrdered);

        outOrdered.push_back(&node);
        return;
    }

    if (node.shape == TsNode::Shape::Enum && !node.typeName.empty())
    {
        auto& key = dedupKey(node);
        if (seen.contains(key))
            return;

        seen.insert(key);
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

// Replaces every non-identifier run in `raw` with a single `_`, then
// strips leading underscores or digits so the result is a legal TS
// identifier. `Ns::Inner::Foo` → `Ns_Inner_Foo`.
std::string sanitizeIdentifier(std::string_view raw)
{
    auto out = std::string {};
    out.reserve(raw.size());

    auto isIdChar = [](char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
               || (c >= '0' && c <= '9') || c == '_';
    };

    for (auto c: raw)
    {
        if (isIdChar(c))
            out += c;
        else if (!out.empty() && out.back() != '_')
            out += '_';
    }

    while (!out.empty() && out.back() == '_')
        out.pop_back();
    while (!out.empty()
           && (out.front() == '_' || (out.front() >= '0' && out.front() <= '9')))
        out.erase(0, 1);

    return out;
}

using NameMap = std::map<std::string, std::string>;

// For each named type collected from the roots, decides what string the
// generator should print for it. When several distinct C++ types share
// an unqualified name (different namespaces), the colliding entries
// fall back to a sanitized qualified name so each declaration is unique.
NameMap chooseOutputNames(const std::vector<const TsNode*>& ordered)
{
    auto byShortName = std::map<std::string, std::vector<const TsNode*>> {};
    for (auto* node: ordered)
        byShortName[node->typeName].push_back(node);

    auto names = NameMap {};
    for (auto& [shortName, group]: byShortName)
    {
        if (group.size() == 1)
        {
            auto* node = group.front();
            if (!node->qualifiedName.empty())
                names[node->qualifiedName] = shortName;
            continue;
        }

        for (auto* node: group)
            names[node->qualifiedName] = sanitizeIdentifier(node->qualifiedName);
    }

    return names;
}

// Walks every TsNode reachable from `root` and rewrites typeName to
// the chosen output name. Re-references to a named type live as
// separate TsNodes (Object/Enum with typeName but no fields), so the
// walk has to descend into fields and inner.
void applyOutputNames(TsNode& root, const NameMap& names)
{
    if (!root.qualifiedName.empty())
    {
        auto it = names.find(root.qualifiedName);
        if (it != names.end())
            root.typeName = it->second;
    }

    for (auto& field: root.fields)
        applyOutputNames(*field.type, names);

    if (root.inner)
        applyOutputNames(*root.inner, names);
}

// ---------- Zod renderer ----------

std::string renderZod(const TsNode& node);

std::string renderZodObjectInline(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "z.object({\n";

    for (auto& field: node.fields)
        out << "    " << formatPropertyKey(field.name) << ": "
            << renderZod(*field.type) << ",\n";

    out << "})";
    return out.str();
}

std::string renderZodEnumInline(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "z.enum([";

    auto first = true;
    for (auto& value: node.enumValues)
    {
        if (!first)
            out << ", ";
        first = false;
        out << "\"" << value << "\"";
    }

    out << "])";
    return out.str();
}

std::string renderZod(const TsNode& node)
{
    auto base = std::string {};

    switch (node.shape)
    {
        case TsNode::Shape::Primitive:
            base = node.primitive.empty() ? "z.unknown()" : node.primitive;
            break;
        case TsNode::Shape::Object:
            base =
                node.typeName.empty() ? renderZodObjectInline(node) : node.typeName;
            break;
        case TsNode::Shape::Array:
            base = "z.array(" + renderZod(*node.inner) + ")";
            break;
        case TsNode::Shape::Map:
            base = "z.record(z.string(), " + renderZod(*node.inner) + ")";
            break;
        case TsNode::Shape::Enum:
            base = node.typeName.empty() ? renderZodEnumInline(node) : node.typeName;
            break;
    }

    if (node.optional)
        base += ".optional()";

    return base;
}

std::string declareZodNamed(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "export const " << node.typeName << " = " << renderZodObjectInline(node)
        << ";\n";
    out << "export type " << node.typeName << " = z.infer<typeof " << node.typeName
        << ">;\n";
    return out.str();
}

std::string declareZodEnum(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "export const " << node.typeName << " = " << renderZodEnumInline(node)
        << ";\n";
    out << "export type " << node.typeName << " = z.infer<typeof " << node.typeName
        << ">;\n";
    return out.str();
}

// ---------- Plain TypeScript renderer ----------

// Renders a node as a TypeScript type expression (no `?` — optionality
// at field-level is handled by the field separator). For non-field
// optional contexts we wrap with ` | undefined`.
std::string renderType(const TsNode& node, bool fieldContext);

std::string renderTypeObjectInline(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "{\n";

    for (auto& field: node.fields)
    {
        auto separator = field.type->optional ? "?: " : ": ";
        out << "    " << formatPropertyKey(field.name) << separator
            << renderType(*field.type, /*fieldContext=*/true) << ";\n";
    }

    out << "}";
    return out.str();
}

std::string renderTypeEnumInline(const TsNode& node)
{
    auto out = std::string {};
    auto first = true;

    for (auto& value: node.enumValues)
    {
        if (!first)
            out += " | ";
        first = false;
        out += "\"";
        out += value;
        out += "\"";
    }

    return out;
}

std::string renderType(const TsNode& node, bool fieldContext)
{
    auto base = std::string {};

    switch (node.shape)
    {
        case TsNode::Shape::Primitive:
            // The primitive slot stores the Zod expression; map to the
            // matching TS scalar.
            if (node.primitive == "z.boolean()")
                base = "boolean";
            else if (node.primitive == "z.string()")
                base = "string";
            else
                base = "number";
            break;
        case TsNode::Shape::Object:
            base =
                node.typeName.empty() ? renderTypeObjectInline(node) : node.typeName;
            break;
        case TsNode::Shape::Array:
            base = renderType(*node.inner, /*fieldContext=*/false) + "[]";
            break;
        case TsNode::Shape::Map:
            base = "Record<string, " + renderType(*node.inner, false) + ">";
            break;
        case TsNode::Shape::Enum:
            base =
                node.typeName.empty() ? renderTypeEnumInline(node) : node.typeName;
            break;
    }

    // In field context, optionality is conveyed by `field?:`. Outside a
    // field (e.g. the inner of an array), optional must be expressed as
    // a union with undefined.
    if (node.optional && !fieldContext)
        base = "(" + base + " | undefined)";

    return base;
}

std::string declareTypeNamed(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "export interface " << node.typeName << " "
        << renderTypeObjectInline(node) << "\n";
    return out.str();
}

std::string declareTypeEnum(const TsNode& node)
{
    auto out = std::ostringstream {};
    out << "export type " << node.typeName << " = " << renderTypeEnumInline(node)
        << ";\n";
    return out.str();
}

// True if the root node was emitted as its own top-level declaration by
// collectNamed — in that case we skip the default-export / Root-alias
// fallback to avoid a redundant second name for the same shape.
bool rootIsHoisted(const TsNode& root)
{
    if (root.typeName.empty())
        return false;

    return root.shape == TsNode::Shape::Object || root.shape == TsNode::Shape::Enum;
}

} // namespace

namespace
{

// Shared bookkeeping for both formatters: walk every root, dedup by
// qualified name, then resolve display-name collisions and rewrite the
// affected typeName fields in place so the renderer can stay simple.
std::vector<const TsNode*> prepareRoots(std::span<TsNode> roots)
{
    auto seen = std::set<std::string> {};
    auto ordered = std::vector<const TsNode*> {};

    for (auto& root: roots)
        collectNamed(root, seen, ordered);

    auto names = chooseOutputNames(ordered);

    for (auto& root: roots)
        applyOutputNames(root, names);

    return ordered;
}

} // namespace

std::string formatZodModule(std::span<TsNode> roots)
{
    auto ordered = prepareRoots(roots);

    auto out = std::ostringstream {};
    out << "import { z } from \"zod\";\n\n";

    for (auto* node: ordered)
    {
        if (node->shape == TsNode::Shape::Enum)
            out << declareZodEnum(*node) << "\n";
        else
            out << declareZodNamed(*node) << "\n";
    }

    return out.str();
}

std::string formatTypesModule(std::span<TsNode> roots)
{
    auto ordered = prepareRoots(roots);

    auto out = std::ostringstream {};

    for (auto* node: ordered)
    {
        if (node->shape == TsNode::Shape::Enum)
            out << declareTypeEnum(*node) << "\n";
        else
            out << declareTypeNamed(*node) << "\n";
    }

    return out.str();
}

std::string formatZodModule(TsNode& root)
{
    auto out = formatZodModule(std::span<TsNode> {&root, 1});

    // The bundled overload skips default exports (one-per-module rule);
    // the single-root path adds one for anonymous roots like top-level
    // vectors so the module isn't pointless.
    if (!rootIsHoisted(root))
        out += "export default " + renderZod(root) + ";\n";

    return out;
}

std::string formatTypesModule(TsNode& root)
{
    auto out = formatTypesModule(std::span<TsNode> {&root, 1});

    if (!rootIsHoisted(root))
        out +=
            "export type Root = " + renderType(root, /*fieldContext=*/false) + ";\n";

    return out;
}

} // namespace Miro::TypeScript
