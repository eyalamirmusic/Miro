#pragma once

#include "Json.h"

#include <string>
#include <string_view>
#include <type_traits>

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
    Reflector(JSON& jsonToUse, bool savingToUse);

    Property operator[](std::string_view key);

    bool isSaving() const;

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

} // namespace Miro
