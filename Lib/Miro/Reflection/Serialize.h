#pragma once

#include "JsonReflector.h"
#include "ReflectContainers.h"
#include "ReflectDispatch.h"

#include <string>
#include <string_view>

namespace Miro
{

template <typename T>
JSON toJSON(const T& value)
{
    auto json = JSON {};
    auto ref = JsonReflector {json, Detail::topLevelOptions<T>(Mode::Save)};
    Detail::reflectValue(ref, const_cast<T&>(value));
    return json;
}

template <typename T>
void fromJSON(T& value, const JSON& json)
{
    auto mutableJson = json;
    auto ref = JsonReflector {mutableJson, Detail::topLevelOptions<T>(Mode::Load)};
    Detail::reflectValue(ref, value);
}

template <typename T>
T createFromJSON(const JSON& json)
{
    auto value = T {};
    fromJSON(value, json);
    return value;
}

template <typename T>
std::string toJSONString(const T& value, int indent = 0)
{
    return Json::print(toJSON(value), indent);
}

template <typename T>
void logJSON(const T& value, int indent = 4)
{
    Json::log(toJSON(value), indent);
}

template <typename T>
void fromJSONString(T& value, std::string_view jsonString)
{
    fromJSON(value, Json::parse(jsonString));
}

template <typename T>
T createFromJSONString(std::string_view jsonString)
{
    return createFromJSON<T>(Json::parse(jsonString));
}

} // namespace Miro
