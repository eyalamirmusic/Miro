#pragma once

#include "Reflector.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Miro::Detail
{

template <typename T>
concept HasReflectMember = requires(T& v, Reflector& r) { v.reflect(r); };

template <typename T>
concept HasExternalReflect = requires(T& v, Reflector& r) { reflect(r, v); };

template <typename T>
concept Reflectable = HasReflectMember<T> || HasExternalReflect<T>;

template <typename T>
void reflectValue(Reflector& ref, T& value);

// Forward declarations for container, optional and enum overloads. The
// definitions live in ReflectContainers.h / ReflectEnum.h, but the
// declarations must be visible here so Phase-1 lookup inside Property's
// templated operator() can see them as candidates.
template <typename T>
void reflectValue(Reflector& ref, std::vector<T>& value);

template <typename T, std::size_t N>
void reflectValue(Reflector& ref, std::array<T, N>& value);

template <typename V>
void reflectValue(Reflector& ref, std::map<std::string, V>& value);

template <typename T>
void reflectValue(Reflector& ref, std::optional<T>& value);

template <typename T>
    requires std::is_enum_v<T>
void reflectValue(Reflector& ref, T& value);

inline void reflectValue(Reflector& ref, bool& value)
{
    ref.visit(value);
}

inline void reflectValue(Reflector& ref, int& value)
{
    ref.visit(value);
}

inline void reflectValue(Reflector& ref, double& value)
{
    ref.visit(value);
}

inline void reflectValue(Reflector& ref, std::string& value)
{
    ref.visit(value);
}

inline void reflectValue(Reflector& ref, std::int64_t& value)
{
    ref.visit(value);
}

template <std::integral T>
    requires(!std::same_as<T, bool> && !std::same_as<T, int>
             && !std::same_as<T, std::int64_t>)
void reflectValue(Reflector& ref, T& value)
{
    auto wide = static_cast<std::int64_t>(value);
    ref.visit(wide);
    value = static_cast<T>(wide);
}

template <std::floating_point T>
    requires(!std::same_as<T, double>)
void reflectValue(Reflector& ref, T& value)
{
    auto wide = static_cast<double>(value);
    ref.visit(wide);
    value = static_cast<T>(wide);
}

// Default fallback: a reflectable struct (member reflect() or external
// `Miro::reflect(Reflector&, T&)` free function). Wraps the user's
// reflect call in asObject(), so the slot becomes a JSON object / a
// schema object before any field is reflected.
template <typename T>
    requires Reflectable<T> && (!std::is_arithmetic_v<T>) && (!std::is_enum_v<T>)
void reflectValue(Reflector& ref, T& value)
{
    ref.asObject(
        [&value](Reflector& sub)
        {
            if constexpr (HasReflectMember<T>)
                value.reflect(sub);
            else
                reflect(sub, value);
        });
}

} // namespace Miro::Detail

namespace Miro
{

template <typename T>
void Property::operator()(T& value)
{
    reflector.atKey(
        key, [&value](Reflector& child) { Detail::reflectValue(child, value); });
}

template <typename T>
void Element::operator()(T& value)
{
    reflector.atIndex(
        index, [&value](Reflector& child) { Detail::reflectValue(child, value); });
}

} // namespace Miro
