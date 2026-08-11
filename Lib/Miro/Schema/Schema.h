#pragma once

#include "../JSON/Json.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

namespace Miro
{

// Roots are passed mutably: the disambiguation pass may rewrite per-node
// `typeName` when distinct C++ types share an unqualified name.
JSON formatJsonSchema(TypeTree::TypeNode& root);
JSON formatJsonSchema(std::span<TypeTree::TypeNode> roots);

template <typename T>
JSON schemaOf()
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
