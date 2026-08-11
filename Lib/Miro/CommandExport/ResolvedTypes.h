#pragma once

#include "../TypeTree/TypeTree.h"
#include "CommandEntry.h"

#include <map>
#include <span>
#include <string>

namespace Miro::CommandExport
{

struct ResolvedTypes
{
    std::map<std::string, std::string> finalNameByQualified;

    // "Empty" means an object type with zero fields.
    std::map<std::string, bool> emptyByQualified;

    std::string nameFor(const std::string& qualified,
                        const std::string& fallback) const;

    bool isEmptyType(const std::string& qualified) const;

    // Also true when the command has no request at all.
    bool isRequestEmpty(const CommandEntry& cmd) const;
};

// Mutates the roots: prepareRoots may rewrite per-node typeName.
ResolvedTypes resolveTypes(std::span<TypeTree::TypeNode> typeRoots);

} // namespace Miro::CommandExport
