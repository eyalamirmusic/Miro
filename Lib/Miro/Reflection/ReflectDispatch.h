#pragma once

#include "Reflector.h"
#include "TypeName.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
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

// Type traits for shape classification.

template <typename T>
struct IsOptional : std::false_type
{
};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type
{
};

template <typename T>
struct InnerOf
{
    using type = T;
};

template <typename T>
struct InnerOf<std::optional<T>>
{
    using type = T;
};

template <typename T>
struct IsArrayLike : std::false_type
{
};

template <typename T>
struct IsArrayLike<std::vector<T>> : std::true_type
{
};

template <typename T, std::size_t N>
struct IsArrayLike<std::array<T, N>> : std::true_type
{
};

template <typename T>
struct IsMapLike : std::false_type
{
};

template <typename V>
struct IsMapLike<std::map<std::string, V>> : std::true_type
{
};

template <typename T>
constexpr bool isOptional()
{
    return IsOptional<T>::value;
}

template <typename T>
consteval Shape shapeOf()
{
    using U = typename InnerOf<T>::type;

    if constexpr (IsArrayLike<U>::value)
        return Shape::Array;
    else if constexpr (IsMapLike<U>::value)
        return Shape::Map;
    else if constexpr (Reflectable<U> && !std::is_arithmetic_v<U>
                       && !std::is_enum_v<U>)
        return Shape::Object;
    else
        return Shape::Primitive;
}

// Build child Options from a parent's Options. mode and schema
// inherit; shape and nullable are decided by the value type T.
template <typename T>
constexpr Options childOptionsFor(const Options& parent)
{
    auto opts = parent;
    opts.shape = shapeOf<T>();
    opts.nullable = isOptional<T>();
    return opts;
}

// Build top-level Options for a fresh root reflector reflecting T.
template <typename T>
constexpr Options topLevelOptions(Mode mode, bool schema = false)
{
    return Options {
        .mode = mode,
        .shape = shapeOf<T>(),
        .nullable = isOptional<T>(),
        .schema = schema,
    };
}

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
// `Miro::reflect(Reflector&, T&)` free function). The slot is already
// committed as Object by the parent's atKey/atIndex via Options.shape,
// so the dispatcher just runs the user's reflect.
template <typename T>
    requires Reflectable<T> && (!std::is_arithmetic_v<T>) && (!std::is_enum_v<T>)
void reflectValue(Reflector& ref, T& value)
{
    if constexpr (isNamedUserType<T>())
        if (!ref.beginNamedType(TypeId {typeNameOf<T>(), rawTypeNameOf<T>()}))
            return;

    if constexpr (HasReflectMember<T>)
        value.reflect(ref);
    else
        reflect(ref, value);
}

} // namespace Miro::Detail

namespace Miro
{

template <typename T>
void Property::operator()(T& value)
{
    Detail::reflectValue(
        reflector.atKey(key, Detail::childOptionsFor<T>(reflector.options())),
        value);
}

template <typename T>
void Element::operator()(T& value)
{
    Detail::reflectValue(
        reflector.atIndex(index, Detail::childOptionsFor<T>(reflector.options())),
        value);
}

} // namespace Miro
