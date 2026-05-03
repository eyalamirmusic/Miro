#pragma once

#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

namespace Miro::TypeScript
{

// The format functions take their roots by mutable reference because
// emission may rewrite per-node `typeName` to disambiguate types from
// different namespaces that share an unqualified name. Callers that
// don't want their trees touched should hand over a copy.
std::string formatZodModule(TypeTree::TypeNode& root);
std::string formatTypesModule(TypeTree::TypeNode& root);

// Multi-root variants — emit one self-contained module that declares
// every named (object or enum) type reachable from any of the roots,
// deduped by qualified name. Used by the type-export runner to bundle
// all registered types into a single .zod.ts / .ts file. The single-
// root versions above add a default export for anonymous roots; the
// bundled versions skip that because a module only allows one default
// export.
std::string formatZodModule(std::span<TypeTree::TypeNode> roots);
std::string formatTypesModule(std::span<TypeTree::TypeNode> roots);

// Public entry points.
template <typename T>
std::string toZod()
{
    auto tree = TypeTree::buildTree<T>();
    return formatZodModule(tree);
}

template <typename T>
std::string toTypes()
{
    auto tree = TypeTree::buildTree<T>();
    return formatTypesModule(tree);
}

} // namespace Miro::TypeScript
