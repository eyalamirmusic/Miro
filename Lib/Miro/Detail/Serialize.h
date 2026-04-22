#pragma once

#include "ReflectContainers.h"
#include "Reflector.h"

#include <string>
#include <string_view>

namespace Miro
{

template <typename T>
JSON toJSON(const T& value)
{
    auto json = JSON(Json::Object {});
    auto ref = Reflector {json, true};
    reflect(ref, const_cast<T&>(value));
    return json;
}

template <typename T>
void fromJSON(T& value, const JSON& json)
{
    auto mutable_json = json;
    auto ref = Reflector {mutable_json, false};
    reflect(ref, value);
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
