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
#include <variant>
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
struct IsOptional : std::false_type
{
};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type
{
};

// A null OwningPointer reflects exactly like an empty optional.
template <typename T>
struct IsOptional<OwningPointer<T>> : std::true_type
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
struct InnerOf<OwningPointer<T>>
{
    using type = T;
};

// Probes for derivation from EA::Vector, so subclasses (EA::OwnedVector,
// user types) are array-shaped without their own specialization.
template <typename U, typename A>
auto vectorDerivedProbe(const Vector<U, A>*) -> std::true_type;
auto vectorDerivedProbe(...) -> std::false_type;

template <typename T>
constexpr bool inheritsFromVector =
    decltype(vectorDerivedProbe(std::declval<T*>()))::value;

template <typename T, typename = void>
struct IsArrayLike : std::false_type
{
};

template <typename T>
struct IsArrayLike<T, std::enable_if_t<inheritsFromVector<T>>> : std::true_type
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

template <typename T, int N>
struct IsArrayLike<Array<T, N>> : std::true_type
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

template <typename V>
struct IsMapLike<EA::MapVector<std::string, V>> : std::true_type
{
};

// Variants serialize as externally tagged objects — `{"Tag": {...}}` —
// so shapeOf reports them as Object.
template <typename T>
struct IsVariant : std::false_type
{
};

template <typename... Ts>
struct IsVariant<std::variant<Ts...>> : std::true_type
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
    else if constexpr (IsVariant<U>::value
                       || (Reflectable<U> && !std::is_arithmetic_v<U>
                           && !std::is_enum_v<U>) )
        return Shape::Object;
    else
        return Shape::Primitive;
}

template <typename T>
constexpr Options childOptionsFor(const Options& parent)
{
    auto opts = parent;
    opts.shape = shapeOf<T>();
    opts.nullable = isOptional<T>();
    return opts;
}

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

// Defined in ReflectContainers.h / ReflectEnum.h, but declared here so
// Phase-1 lookup in Property::operator() finds them as candidates.
template <typename T>
void reflectValue(Reflector& ref, std::vector<T>& value);

template <typename T, std::size_t N>
void reflectValue(Reflector& ref, std::array<T, N>& value);

template <typename V>
void reflectValue(Reflector& ref, std::map<std::string, V>& value);

template <typename T>
void reflectValue(Reflector& ref, std::optional<T>& value);

template <typename T, typename Allocator>
void reflectValue(Reflector& ref, Vector<T, Allocator>& value);

template <typename T, int N>
void reflectValue(Reflector& ref, Array<T, N>& value);

template <typename V>
void reflectValue(Reflector& ref, EA::MapVector<std::string, V>& value);

template <typename T>
void reflectValue(Reflector& ref, OwningPointer<T>& value);

template <typename... Ts>
void reflectValue(Reflector& ref, std::variant<Ts...>& value);

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

template <typename T>
    requires Reflectable<T> && (!std::is_arithmetic_v<T>) && (!std::is_enum_v<T>)
void reflectValue(Reflector& ref, T& value)
{
    if constexpr (isNamedUserType<T>())
        if (!ref.beginNamedType(TypeId {typeNameOf<T>(), qualifiedNameOf<T>()}))
            return;

    if constexpr (HasReflectMember<T>)
        value.reflect(ref);
    else
        reflect(ref, value);
}

} // namespace Miro::Detail

namespace Miro
{

// The call to reflectValue stays unqualified so ADL re-runs at
// instantiation: users can add their own reflectValue(Reflector&, T&)
// overload in namespace Miro or in T's namespace, even after including us.
template <typename T>
void Property::operator()(T& value)
{
    using Detail::reflectValue;
    reflectValue(reflector.atKey(key,
                                 Detail::childOptionsFor<T>(reflector.options())),
                 value);
}

template <typename T>
void Element::operator()(T& value)
{
    using Detail::reflectValue;
    reflectValue(reflector.atIndex(index,
                                   Detail::childOptionsFor<T>(reflector.options())),
                 value);
}

} // namespace Miro
