#include "Reflector.h"

#include <stdexcept>
#include <string>

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

void Reflector::requirePolymorphicSupport(std::string_view context)
{
    auto message = std::string {"Reflector does not support polymorphic dispatch ("};
    message += context;
    message += ").";
    throw std::logic_error(message);
}

} // namespace Miro
