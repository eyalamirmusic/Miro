#pragma once

#include "JsonReflector.h"
#include "ReflectContainers.h"
#include "ReflectDispatch.h"
#include "TypeName.h"

#include <string>
#include <string_view>
#include <utility>

namespace Miro
{

template <typename T>
JSON toJSON(const T& value, CustomOptions custom = {})
{
    auto json = JSON {};
    auto ref = JsonReflector {
        json,
        Detail::topLevelOptions<T>(Mode::Save, /*schema=*/false, std::move(custom))};
    Detail::reflectValue(ref, const_cast<T&>(value));
    return json;
}

// Never throws. Missing keys and type mismatches leave the corresponding
// field untouched, and a fault leaves `value` partially populated.
template <typename T>
void fromJSON(T& value, const JSON& json, CustomOptions custom = {})
{
    try
    {
        auto mutableJson = json;
        auto ref =
            JsonReflector {mutableJson,
                           Detail::topLevelOptions<T>(
                               Mode::Load, /*schema=*/false, std::move(custom))};
        Detail::reflectValue(ref, value);
    }
    catch (...)
    {
    }
}

template <typename T>
T createFromJSON(const JSON& json, CustomOptions custom = {})
{
    auto value = T {};
    fromJSON(value, json, std::move(custom));
    return value;
}

template <typename T>
std::string toJSONString(const T& value, int indent = 0, CustomOptions custom = {})
{
    return Json::print(toJSON(value, std::move(custom)), indent);
}

template <typename T>
void logJSON(const T& value, int indent = 4, CustomOptions custom = {})
{
    Json::log(toJSON(value, std::move(custom)), indent);
}

template <typename T>
void fromJSONString(T& value, std::string_view jsonString, CustomOptions custom = {})
{
    fromJSON(value, Json::getParsedValue(jsonString), std::move(custom));
}

template <typename T>
T createFromJSONString(std::string_view jsonString, CustomOptions custom = {})
{
    return createFromJSON<T>(Json::getParsedValue(jsonString), std::move(custom));
}

} // namespace Miro
