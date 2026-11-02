#pragma once

#include "../TOML/Toml.h"
#include "Serialize.h"

#include <string>
#include <string_view>
#include <utility>

// TOML counterparts of the JSON string helpers in Serialize.h. A struct
// goes through toJSON / fromJSON and the Json::Value is printed as, or
// parsed from, TOML, so keys, enums, maps, optionals and Omittable
// behave exactly as they do in JSON. Unlike fromJSONString, the load
// side throws Toml::ParseError for a malformed document, because a
// configuration file that silently loads as defaults hides the mistake.

namespace Miro
{

template <typename T>
std::string toTOMLString(const T& value, CustomOptions custom = {})
{
    return Toml::print(toJSON(value, std::move(custom)));
}

template <typename T>
void fromTOMLString(T& value, std::string_view tomlString, CustomOptions custom = {})
{
    fromJSON(value, Toml::parse(tomlString), std::move(custom));
}

template <typename T>
T createFromTOMLString(std::string_view tomlString, CustomOptions custom = {})
{
    return createFromJSON<T>(Toml::parse(tomlString), std::move(custom));
}

} // namespace Miro
