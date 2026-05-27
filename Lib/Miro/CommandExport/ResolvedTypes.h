#pragma once

#include "../TypeTree/TypeTree.h"
#include "CommandEntry.h"

#include <map>
#include <span>
#include <string>

namespace Miro::CommandExport
{

// Per-type info derived from a set of TypeNode roots after the
// disambiguation pass has run. Keyed by raw qualified C++ name
// (matches CommandEntry::*QualifiedName) so callers don't need to
// know how collisions were resolved.
struct ResolvedTypes
{
    // Final TypeScript / C++ type name to emit, after collision
    // rewrites in TypeTree::prepareRoots.
    std::map<std::string, std::string> finalNameByQualified;

    // True for object types with zero fields — used to elide the
    // request parameter on the matching command's emitted signature.
    std::map<std::string, bool> emptyByQualified;

    std::string nameFor(const std::string& qualified,
                        const std::string& fallback) const;

    bool isEmptyType(const std::string& qualified) const;

    // True when the command's request type is empty (or absent),
    // i.e. callers can drop the param at the JS / C++ callsite.
    bool isRequestEmpty(const CommandEntry& cmd) const;
};

// Runs TypeTree::prepareRoots over the roots (which may rewrite
// per-node typeName) and indexes the result by qualified name.
ResolvedTypes resolveTypes(std::span<TypeTree::TypeNode> typeRoots);

} // namespace Miro::CommandExport
