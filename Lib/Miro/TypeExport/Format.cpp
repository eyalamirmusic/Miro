#include "Format.h"

namespace Miro::TypeExport
{

namespace Detail
{

Vector<Format>& formatRegistry()
{
    static auto registry = Vector<Format> {};
    return registry;
}

} // namespace Detail

int registerFormat(Format format)
{
    Detail::formatRegistry().add(std::move(format));
    return 0;
}

Vector<TypeTree::TypeNode> buildAllTypeTrees(const EntryList& entries)
{
    auto roots = Vector<TypeTree::TypeNode> {};
    roots.reserve(entries.size());

    for (auto& entry: entries)
    {
        auto opts = entry->topLevelOptions(Mode::Save, /*schema=*/true);
        auto& root = roots.emplace_back();
        auto refl = TypeTree::TypeReflector {root, opts};
        entry->reflect(refl);
    }

    return roots;
}

} // namespace Miro::TypeExport
