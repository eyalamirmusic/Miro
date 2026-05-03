#pragma once

#include "../JSON/Json.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

namespace Miro
{

// Renders a TypeNode tree (or several roots) as a JSON Schema document.
// Every named type's body lives under a top-level "$defs" map; every
// reference (including recursive ones) becomes {"$ref": "..."}.
//
// Roots are passed mutably because the disambiguation pass may rewrite
// per-node `typeName` when distinct C++ types share an unqualified
// name. Callers that don't want the trees touched should hand over a
// copy.
Json::Value formatJsonSchema(TypeTree::TypeNode& root);
Json::Value formatJsonSchema(std::span<TypeTree::TypeNode> roots);

template <typename T>
Json::Value schemaOf()
{
    auto tree = TypeTree::buildTree<T>();
    return formatJsonSchema(tree);
}

template <typename T>
std::string schemaOfAsString(int indent = 2)
{
    return Json::print(schemaOf<T>(), indent);
}

} // namespace Miro
