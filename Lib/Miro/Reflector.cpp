#include "Reflector.h"

namespace Miro
{

Reflector::Reflector(JSON& jsonToUse, bool savingToUse)
    : json(jsonToUse)
    , saving(savingToUse)
{
}

Property Reflector::operator[](std::string_view key)
{
    return {*this, key};
}

bool Reflector::isSaving() const
{
    return saving;
}

} // namespace Miro
