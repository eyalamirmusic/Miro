#include "Register.h"

namespace Miro::TypeExport::Detail
{

std::vector<std::unique_ptr<TypeEntry>>& registry()
{
    static auto entries = std::vector<std::unique_ptr<TypeEntry>> {};
    return entries;
}

} // namespace Miro::TypeExport::Detail
