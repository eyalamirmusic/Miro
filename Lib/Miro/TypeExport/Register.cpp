#include "Register.h"

namespace Miro::TypeExport::Detail
{

OwnedVector<TypeEntry>& registry()
{
    static auto entries = OwnedVector<TypeEntry> {};
    return entries;
}

} // namespace Miro::TypeExport::Detail
