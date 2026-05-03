#include "TypeTree.h"

#include <concepts>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

namespace Miro::TypeTree
{

// ----- TypeReflector --------------------------------------------------

TypeReflector::TypeReflector(TypeNode& nodeToUse,
                             Options optsToUse,
                             TypeReflector* parentToUse)
    : Reflector(optsToUse)
    , node(nodeToUse)
    , parent(parentToUse)
{
    switch (opts.shape)
    {
        case Miro::Shape::Primitive:
            node.shape = TypeNode::Shape::Primitive;
            break;
        case Miro::Shape::Object:
            node.shape = TypeNode::Shape::Object;
            break;
        case Miro::Shape::Array:
            node.shape = TypeNode::Shape::Array;
            node.inner = std::make_unique<TypeNode>();
            break;
        case Miro::Shape::Map:
            node.shape = TypeNode::Shape::Map;
            node.inner = std::make_unique<TypeNode>();
            break;
    }

    node.optional = opts.nullable;
}

TypeReflector::~TypeReflector() = default;

ValueKind TypeReflector::kind() const
{
    return ValueKind::Absent;
}

void TypeReflector::writeNull() {}

bool TypeReflector::beginNamedType(TypeId id)
{
    // Set the names regardless: even when this is a back edge, the
    // renderer needs them to print the type by name.
    node.typeName = std::string {id.shortName};
    node.qualifiedName = std::string {id.qualifiedName};

    // Walk the parent chain — if any ancestor is currently inside a body
    // for the same C++ type, this slot is a back edge and we must not
    // descend or we'll infinite-recurse. The slot becomes a name
    // reference; collectNamed dedups against the outer walk.
    for (auto* p = parent; p != nullptr; p = p->parent)
    {
        if (p->activeQualifiedName == id.qualifiedName)
            return false;
    }

    activeQualifiedName = std::string {id.qualifiedName};
    return true;
}

void TypeReflector::visitEnum(TypeId id, const std::vector<std::string_view>& names)
{
    node.shape = TypeNode::Shape::Enum;
    node.typeName = std::string {id.shortName};
    node.qualifiedName = std::string {id.qualifiedName};
    node.enumValues.clear();
    node.enumValues.reserve(names.size());

    for (auto& name: names)
        node.enumValues.emplace_back(name);
}

void TypeReflector::visit(PrimitiveRef ref)
{
    std::visit(
        [this](auto* ptr)
        {
            using T = std::remove_pointer_t<decltype(ptr)>;
            if constexpr (std::same_as<T, bool>)
                node.primitive = PrimitiveKind::Boolean;
            else if constexpr (std::same_as<T, std::string>)
                node.primitive = PrimitiveKind::String;
            else if constexpr (std::same_as<T, double>)
                node.primitive = PrimitiveKind::Number;
            else
                node.primitive = PrimitiveKind::Integer;
        },
        ref.data);
}

Reflector& TypeReflector::spawnChild(TypeNode& targetNode, Options childOpts)
{
    currentChild.reset();
    currentChild = std::make_unique<TypeReflector>(targetNode, childOpts, this);
    return *currentChild;
}

Reflector& TypeReflector::atKey(std::string_view key, Options childOpts)
{
    if (node.shape == TypeNode::Shape::Map)
        return spawnChild(*node.inner, childOpts);

    auto field = TypeNode::Field {std::string {key}, std::make_unique<TypeNode>()};
    node.fields.push_back(std::move(field));
    return spawnChild(*node.fields.back().type, childOpts);
}

Reflector& TypeReflector::atIndex(std::size_t, Options childOpts)
{
    return spawnChild(*node.inner, childOpts);
}

void TypeReflector::setArrayBounds(std::size_t min, std::size_t max)
{
    node.minItems = min;
    node.maxItems = max;
}

// ----- prepareRoots and helpers ---------------------------------------

namespace
{

// Identity used to dedup a node in collectNamed. Prefers the raw
// qualified name (always unique per C++ type); falls back to the short
// name for the rare anonymous-object case so an unnamed slot still
// pairs with itself if encountered twice.
const std::string& dedupKey(const TypeNode& node)
{
    return node.qualifiedName.empty() ? node.typeName : node.qualifiedName;
}

// Collects every named (Object/Enum) node reachable from `node` in
// post-order (deepest first). Re-encounters of the same C++ type (by
// qualified name) become inline name references in rendered output.
void collectNamed(const TypeNode& node,
                  std::set<std::string>& seen,
                  std::vector<const TypeNode*>& outOrdered)
{
    if (node.shape == TypeNode::Shape::Object && !node.typeName.empty())
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

    if (node.shape == TypeNode::Shape::Enum && !node.typeName.empty())
    {
        auto& key = dedupKey(node);
        if (seen.contains(key))
            return;

        seen.insert(key);
        outOrdered.push_back(&node);
        return;
    }

    if (node.shape == TypeNode::Shape::Object)
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
// strips leading underscores or digits so the result is a legal
// identifier. `Ns::Inner::Foo` → `Ns_Inner_Foo`. Strips a leading
// "(anonymous namespace)::" so test types in anonymous namespaces
// produce clean names.
std::string sanitizeIdentifier(std::string_view raw)
{
    constexpr auto anonPrefix = std::string_view {"(anonymous namespace)::"};
    auto trimmed = raw;
    while (trimmed.starts_with(anonPrefix))
        trimmed.remove_prefix(anonPrefix.size());

    auto isIdChar = [](char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
               || (c >= '0' && c <= '9') || c == '_';
    };

    auto out = std::string {};
    out.reserve(trimmed.size());

    for (auto c: trimmed)
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

// For each named type collected from the roots, decides what string
// will be emitted for it. When several distinct C++ types share an
// unqualified name (different namespaces), the colliding entries fall
// back to a sanitized qualified name so each declaration is unique.
NameMap chooseOutputNames(const std::vector<const TypeNode*>& ordered)
{
    auto byShortName = std::map<std::string, std::vector<const TypeNode*>> {};
    for (auto* n: ordered)
        byShortName[n->typeName].push_back(n);

    auto names = NameMap {};
    for (auto& [shortName, group]: byShortName)
    {
        if (group.size() == 1)
        {
            auto* n = group.front();
            if (!n->qualifiedName.empty())
                names[n->qualifiedName] = shortName;
            continue;
        }

        for (auto* n: group)
            names[n->qualifiedName] = sanitizeIdentifier(n->qualifiedName);
    }

    return names;
}

// Walks every TypeNode reachable from `root` and rewrites typeName to
// the chosen output name. References to a named type live as separate
// TypeNodes (Object/Enum with typeName but no fields), so the walk has
// to descend into fields and inner.
void applyOutputNames(TypeNode& root, const NameMap& names)
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

} // namespace

std::vector<const TypeNode*> prepareRoots(std::span<TypeNode> roots)
{
    auto seen = std::set<std::string> {};
    auto ordered = std::vector<const TypeNode*> {};

    for (auto& root: roots)
        collectNamed(root, seen, ordered);

    auto names = chooseOutputNames(ordered);

    for (auto& root: roots)
        applyOutputNames(root, names);

    return ordered;
}

} // namespace Miro::TypeTree
