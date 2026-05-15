#include "ResolvedTypes.h"

namespace Miro::CommandExport
{

std::string ResolvedTypes::nameFor(const std::string& qualified,
                                   const std::string& fallback) const
{
    auto it = finalNameByQualified.find(qualified);

    if (it != finalNameByQualified.end())
        return it->second;

    return fallback;
}

bool ResolvedTypes::isEmptyType(const std::string& qualified) const
{
    auto it = emptyByQualified.find(qualified);
    return it != emptyByQualified.end() && it->second;
}

bool ResolvedTypes::isRequestEmpty(const CommandEntry& cmd) const
{
    if (!cmd.hasRequest)
        return true;

    return isEmptyType(cmd.requestQualifiedName);
}

ResolvedTypes resolveTypes(std::span<TypeTree::TypeNode> typeRoots)
{
    TypeTree::prepareRoots(typeRoots);

    auto resolved = ResolvedTypes {};

    for (auto& root: typeRoots)
    {
        if (root.qualifiedName.empty())
            continue;

        resolved.finalNameByQualified[root.qualifiedName] = root.typeName;
        resolved.emptyByQualified[root.qualifiedName] =
            root.shape == TypeTree::TypeNode::Shape::Object && root.fields.empty();
    }

    return resolved;
}

} // namespace Miro::CommandExport
