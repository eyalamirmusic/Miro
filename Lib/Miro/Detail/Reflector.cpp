#include "Reflector.h"

namespace Miro
{

Property Reflector::operator[](std::string_view key)
{
    return {*this, key};
}

Element Reflector::operator[](std::size_t index)
{
    return {*this, index};
}

} // namespace Miro
