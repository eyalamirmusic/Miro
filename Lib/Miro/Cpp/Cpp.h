#pragma once

#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

namespace Miro::Cpp
{

// Miro mode adds MIRO_REFLECT(...) and an <Miro/Miro.h> include so the
// generated structs are serializable; PureCPP has no Miro dependency.
enum class Modes
{
    Miro,
    PureCPP
};

// Roots are mutable: the disambiguation pass rewrites per-node `typeName`
// when distinct types share a short name.
std::string formatHeader(TypeTree::TypeNode& root, Modes mode = Modes::PureCPP);
std::string formatHeader(std::span<TypeTree::TypeNode> roots,
                         Modes mode = Modes::PureCPP);

template <typename T>
std::string toHeader(Modes mode = Modes::PureCPP)
{
    auto tree = TypeTree::buildTree<T>();
    return formatHeader(tree, mode);
}

} // namespace Miro::Cpp
