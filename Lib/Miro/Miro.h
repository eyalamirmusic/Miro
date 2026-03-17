#pragma once

#include "Json.h"

#include <string>
#include <string_view>
#include <array>
#include <vector>

namespace Miro
{
struct Reflector;

struct Property
{
    template <typename T>
    void operator()(T& value);

    Reflector& reflector;
    std::string_view key;
};

struct Reflector
{
    Reflector(JSON& jsonToUse, bool savingToUse)
        : json(jsonToUse)
        , saving(savingToUse)
    {
    }

    Property operator[](std::string_view key) { return {*this, key}; }

    bool isSaving() const { return saving; }

    JSON& json;
    bool saving;
};

template <typename T>
constexpr bool isPrimitive()
{
    return std::is_same_v<T, bool> || std::is_same_v<T, int>
           || std::is_same_v<T, double> || std::is_same_v<T, std::string>;
}

template <typename T>
void reflectPrimitive(Reflector& ref, T& value)
{
    if (ref.isSaving())
        ref.json = JSON(value);
    else
        value = static_cast<T>(ref.json);
}

template <typename T>
void reflect(Reflector& ref, T& value)
{
    if constexpr (isPrimitive<T>())
        reflectPrimitive(ref, value);
    else
        value.reflect(ref);
}

template <typename T>
void reflect(Reflector& ref, std::vector<T>& value)
{
    if (ref.isSaving())
    {
        auto& arr = ref.json.data.emplace<Json::Array>();

        for (auto& element: value)
        {
            auto& node = arr.emplace_back();
            auto child = Reflector {node, true};
            reflect(child, element);
        }
    }
    else
    {
        auto& arr = std::get<Json::Array>(ref.json.data);
        value.resize(arr.size());

        for (std::size_t i = 0; i < arr.size(); ++i)
        {
            auto child = Reflector {arr[i], false};
            reflect(child, value[i]);
        }
    }
}

template <typename T, std::size_t N>
void reflect(Reflector& ref, std::array<T, N>& value)
{
    if (ref.isSaving())
    {
        auto& arr = ref.json.data.emplace<Json::Array>();

        for (auto& element: value)
        {
            auto& node = arr.emplace_back();
            auto child = Reflector {node, true};
            reflect(child, element);
        }
    }
    else
    {
        auto& arr = std::get<Json::Array>(ref.json.data);
        auto count = std::min(N, arr.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            auto child = Reflector {arr[i], false};
            reflect(child, value[i]);
        }
    }
}

template <typename T>
void reflect(Reflector& ref, std::string_view key, T& value)
{
    auto& obj = ref.json.toObject();

    if (ref.isSaving())
    {
        auto child = Reflector {obj[std::string(key)], true};
        reflect(child, value);
    }
    else
    {
        auto it = obj.find(std::string(key));

        if (it != obj.end())
        {
            auto child = Reflector {it->second, false};
            reflect(child, value);
        }
    }
}

template <typename T>
void Property::operator()(T& value)
{
    Miro::reflect(reflector, key, value);
}

template <typename T>
JSON toJSON(T& value)
{
    auto json = JSON(Json::Object {});
    auto ref = Reflector {json, true};
    reflect(ref, value);
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
std::string toJSONString(T& value)
{
    return Json::print(toJSON(value));
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
