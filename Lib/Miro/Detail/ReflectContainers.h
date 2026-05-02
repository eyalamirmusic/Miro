#pragma once

#include "ReflectDispatch.h"

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Miro::Detail
{

template <typename T>
void reflectVectorBody(Reflector& ref, std::vector<T>& value)
{
    if (ref.isSaving())
    {
        if (ref.isSchema())
        {
            auto inner = T {};
            ref.atIndex(0,
                        [&inner](Reflector& child) { reflectValue(child, inner); });
        }
        else
        {
            ref.resizeArray(value.size());
            for (std::size_t i = 0; i < value.size(); ++i)
                ref.atIndex(i,
                            [&value, i](Reflector& child)
                            { reflectValue(child, value[i]); });
        }
    }
    else
    {
        value.resize(ref.arraySize());
        for (std::size_t i = 0; i < value.size(); ++i)
            ref.atIndex(
                i, [&value, i](Reflector& child) { reflectValue(child, value[i]); });
    }
}

template <typename T, std::size_t N>
void reflectArrayBody(Reflector& ref, std::array<T, N>& value)
{
    if (ref.isSaving())
    {
        if (ref.isSchema())
        {
            auto inner = T {};
            ref.atIndex(0,
                        [&inner](Reflector& child) { reflectValue(child, inner); });
        }
        else
        {
            ref.resizeArray(N);
            for (std::size_t i = 0; i < N; ++i)
                ref.atIndex(i,
                            [&value, i](Reflector& child)
                            { reflectValue(child, value[i]); });
        }
    }
    else
    {
        auto size = ref.arraySize();
        auto count = size < N ? size : N;
        for (std::size_t i = 0; i < count; ++i)
            ref.atIndex(
                i, [&value, i](Reflector& child) { reflectValue(child, value[i]); });
    }
}

template <typename V>
void reflectMapBody(Reflector& ref, std::map<std::string, V>& value)
{
    if (ref.isSaving())
    {
        if (ref.isSchema())
        {
            auto inner = V {};
            ref.atKey("*",
                      [&inner](Reflector& child) { reflectValue(child, inner); });
        }
        else
        {
            for (auto& [key, element]: value)
                ref.atKey(key,
                          [&element](Reflector& child)
                          { reflectValue(child, element); });
        }
    }
    else
    {
        value.clear();
        for (auto& key: ref.mapKeys())
        {
            auto element = V {};
            ref.atKey(
                key, [&element](Reflector& child) { reflectValue(child, element); });
            value.emplace(key, std::move(element));
        }
    }
}

template <typename T>
void reflectValue(Reflector& ref, std::vector<T>& value)
{
    ref.asArray([&value](Reflector& sub) { reflectVectorBody(sub, value); });
}

template <typename T, std::size_t N>
void reflectValue(Reflector& ref, std::array<T, N>& value)
{
    ref.asArray([&value](Reflector& sub) { reflectArrayBody(sub, value); });
}

template <typename V>
void reflectValue(Reflector& ref, std::map<std::string, V>& value)
{
    ref.asMap([&value](Reflector& sub) { reflectMapBody(sub, value); });
}

template <typename T>
void reflectValue(Reflector& ref, std::optional<T>& value)
{
    ref.markNullable();

    if (ref.isSaving())
    {
        if (ref.isSchema())
        {
            auto inner = T {};
            reflectValue(ref, inner);
        }
        else if (value)
        {
            reflectValue(ref, *value);
        }
        else
        {
            ref.writeNull();
        }
    }
    else
    {
        auto k = ref.kind();

        if (k == ValueKind::Null)
        {
            value.reset();
        }
        else if (k != ValueKind::Absent)
        {
            auto inner = T {};
            reflectValue(ref, inner);
            value = std::move(inner);
        }
    }
}

} // namespace Miro::Detail
