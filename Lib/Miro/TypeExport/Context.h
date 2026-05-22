#pragma once

// Bundle of everything a Format functor needs to render. Decouples
// the format pipeline from how the data was sourced — the same
// formats run against:
//   - the static-init process-wide registries (existing main.cpp)
//   - a DescribeReflector walk over one or more API classes
//     (codegenMain<Apis...> — the inversion-of-control path)
//
// typeRoots is taken mutably because rendering may rewrite per-node
// typeName during the disambiguation pass (matches the existing
// formatZodModule / formatTypesModule contract).

#include "../CommandExport/Register.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string_view>

namespace Miro::TypeExport
{

struct Context
{
    std::span<TypeTree::TypeNode> typeRoots;
    std::span<const CommandExport::CommandEntry> commands;
    std::string_view baseName;
};

} // namespace Miro::TypeExport
