#pragma once

#include "../TypeExport/Context.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>
#include <string_view>

namespace Miro::TypeScript
{

// Roots are taken mutably: emission rewrites per-node `typeName` to
// disambiguate same-named types from different namespaces.
std::string formatZodModule(TypeTree::TypeNode& root);
std::string formatTypesModule(TypeTree::TypeNode& root);

// The bundled overloads add no default export (a module allows only one),
// unlike the single-root ones above.
std::string formatZodModule(std::span<TypeTree::TypeNode> roots);
std::string formatTypesModule(std::span<TypeTree::TypeNode> roots);

std::string formatBridgeRuntime();

std::string formatEventsModule(std::span<TypeTree::TypeNode> typeRoots,
                               std::span<const TypeExport::EventInfo> events,
                               std::string_view baseName);

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
