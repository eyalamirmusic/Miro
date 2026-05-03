#pragma once

#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

// Emits a C++ header that declares the same shapes as the source types,
// using `std::vector`, `std::map<std::string, V>`, `std::optional`,
// `std::string`, plain `int` / `double` / `bool`, and `enum class` for
// enums. Useful when you want to consume Miro-described types from
// another C++ project (e.g. a server, a public API) without depending
// on the original source.
//
// Two flavours, picked at the call site:
//   - `withMiro = false`: pure types, zero Miro dependency.
//   - `withMiro = true` : the same shapes plus `MIRO_REFLECT(...)` and
//     an `#include <Miro/Miro.h>`, so the generated types are
//     immediately serializable.
//
// Roots are mutable because the TypeTree disambiguation pass rewrites
// per-node `typeName` when distinct types share a short name.

namespace Miro::Cpp
{

std::string formatHeader(TypeTree::TypeNode& root, bool withMiro);
std::string formatHeader(std::span<TypeTree::TypeNode> roots, bool withMiro);

template <typename T>
std::string toHeader(bool withMiro)
{
    auto tree = TypeTree::buildTree<T>();
    return formatHeader(tree, withMiro);
}

} // namespace Miro::Cpp
