#pragma once

#include <cmath>
#include <concepts>
#include <cstdlib>
#include <sstream>
#include <string>

namespace Miro
{
template <typename T>
std::string primitiveToString(T* ptr)
{
    if constexpr (std::same_as<T, bool>)
    {
        return *ptr ? "true" : "false";
    }
    else if constexpr (std::same_as<T, std::string>)
    {
        return *ptr;
    }
    else if constexpr (std::same_as<T, double>)
    {
        // Match JSON printer behavior — emit integers without a decimal
        // point so round-trips through int-backed reflect targets don't
        // pick up "5.0" strings.
        if (std::isfinite(*ptr) && *ptr == std::floor(*ptr) && std::abs(*ptr) < 1e15)
        {
            auto stream = std::ostringstream {};
            stream << static_cast<long long>(*ptr);
            return stream.str();
        }

        auto stream = std::ostringstream {};
        stream << *ptr;
        return stream.str();
    }
    else
    {
        return std::to_string(*ptr);
    }
}

template <typename T>
void primitiveFromString(const std::string& text, T* ptr)
{
    if constexpr (std::same_as<T, bool>)
    {
        *ptr = (text == "true" || text == "1");
    }
    else if constexpr (std::same_as<T, std::string>)
    {
        *ptr = text;
    }
    else if constexpr (std::same_as<T, double>)
    {
        *ptr = std::strtod(text.c_str(), nullptr);
    }
    else
    {
        *ptr = static_cast<T>(std::strtoll(text.c_str(), nullptr, 10));
    }
}
} // namespace Miro
