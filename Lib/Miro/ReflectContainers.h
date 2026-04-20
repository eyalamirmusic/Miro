#pragma once

#include "Reflector.h"

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace Miro
{

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

template <typename V>
void reflect(Reflector& ref, std::map<std::string, V>& value)
{
    if (ref.isSaving())
    {
        auto& obj = ref.json.data.emplace<Json::Object>();

        for (auto& [key, element]: value)
        {
            auto child = Reflector {obj[key], true};
            reflect(child, element);
        }
    }
    else
    {
        auto& obj = std::get<Json::Object>(ref.json.data);
        value.clear();

        for (auto& [key, node]: obj)
        {
            auto child = Reflector {node, false};
            reflect(child, value[key]);
        }
    }
}

} // namespace Miro
