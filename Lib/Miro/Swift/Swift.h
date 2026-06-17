#pragma once

#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

// Emits a Swift source file declaring the same shapes as the source
// types:
//   - object -> `struct Name: Codable { ... }`
//   - enum   -> `enum Name: String, Codable { case ... }`
//   - vector -> `[T]`, map -> `[String: V]`, optional -> `T?`
//   - primitives -> Bool / String / Double / Int / Int64
//
// Enums serialize by name (matching Miro's JSON wire format), so each
// case carries an explicit String raw value. Fields whose JSON key is
// not a bare Swift identifier get a generated `CodingKeys` mapping that
// restores the original wire key.
//
// The generated file is meant to be compiled into the consuming target
// (default `internal` access, relying on the synthesized memberwise and
// Codable initializers), not shipped as a standalone public module.
//
// Roots are mutable because the TypeTree disambiguation pass rewrites
// per-node `typeName` when distinct types share a short name.

namespace Miro::Swift
{

std::string formatTypes(TypeTree::TypeNode& root);
std::string formatTypes(std::span<TypeTree::TypeNode> roots);

template <typename T>
std::string toSource()
{
    auto tree = TypeTree::buildTree<T>();
    return formatTypes(tree);
}

} // namespace Miro::Swift
