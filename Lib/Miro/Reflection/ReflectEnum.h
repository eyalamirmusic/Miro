#pragma once

#include "Reflector.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Miro
{

// Specialize this to shrink or extend the probed enumerator range for a
// given enum type. The default covers [-128, 127].
//
// Note: an unscoped enum without a fixed underlying type
// (e.g. `enum Foo { ... }`) has a value range determined by its
// enumerators, and casting out-of-range values in a constant expression
// is UB. Give such enums an explicit base (`enum Foo : int { ... }`) or
// use `enum class` to reflect them.
template <typename E>
struct EnumRange
{
    static constexpr int minValue = -128;
    static constexpr int maxValue = 127;
};

namespace Detail
{

template <auto V>
constexpr std::string_view enumNameRaw()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr auto prefix = std::string_view {"V = "};
    constexpr auto terminators = std::string_view {";]"};
    auto sig = std::string_view {__PRETTY_FUNCTION__};
    auto start = sig.find(prefix) + prefix.size();
    auto end = sig.find_first_of(terminators, start);
#elif defined(_MSC_VER)
    constexpr auto prefix = std::string_view {"enumNameRaw<"};
    constexpr auto terminator = std::string_view {">("};
    auto sig = std::string_view {__FUNCSIG__};
    auto start = sig.find(prefix) + prefix.size();
    auto end = sig.find(terminator, start);
#else
    auto sig = std::string_view {};
    auto start = std::size_t {};
    auto end = std::size_t {};
#endif

    auto name = sig.substr(start, end - start);
    auto colon = name.rfind(':');

    if (colon != std::string_view::npos)
        name.remove_prefix(colon + 1);

    if (!name.empty() && name.front() == '(')
        return {};

    return name;
}

template <typename E, int Min, std::size_t... Is>
constexpr auto buildEnumTable(std::index_sequence<Is...>)
{
    return std::array<std::pair<E, std::string_view>, sizeof...(Is)> {
        std::pair {static_cast<E>(Min + static_cast<int>(Is)),
                   enumNameRaw<static_cast<E>(Min + static_cast<int>(Is))>()}...};
}

template <typename E>
    requires std::is_enum_v<E>
inline constexpr auto enumTable = buildEnumTable<E, EnumRange<E>::minValue>(
    std::make_index_sequence<static_cast<std::size_t>(
        EnumRange<E>::maxValue - EnumRange<E>::minValue + 1)> {});

} // namespace Detail

template <typename E>
    requires std::is_enum_v<E>
constexpr std::string_view enumToString(E value)
{
    for (auto& [entryValue, name]: Detail::enumTable<E>)
        if (entryValue == value && !name.empty())
            return name;

    return {};
}

template <typename E>
    requires std::is_enum_v<E>
constexpr std::optional<E> enumFromString(std::string_view str)
{
    for (auto& [entryValue, name]: Detail::enumTable<E>)
        if (!name.empty() && name == str)
            return entryValue;

    return std::nullopt;
}

namespace Detail
{

template <typename T>
    requires std::is_enum_v<T>
void reflectValue(Reflector& ref, T& value)
{
    using Underlying = std::underlying_type_t<T>;

    if (ref.isSaving())
    {
        if (ref.isSchema())
        {
            auto placeholder = std::string {"enum"};
            ref.visit(placeholder);
        }
        else if (auto name = std::string {enumToString(value)}; !name.empty())
        {
            ref.visit(name);
        }
        else
        {
            auto numeric = static_cast<std::int64_t>(static_cast<Underlying>(value));
            ref.visit(numeric);
        }
    }
    else
    {
        auto kind = ref.kind();

        if (kind == ValueKind::String)
        {
            auto name = std::string {};
            ref.visit(name);

            if (auto parsed = enumFromString<T>(name))
                value = *parsed;
        }
        else if (kind == ValueKind::Number)
        {
            auto numeric = std::int64_t {};
            ref.visit(numeric);
            value = static_cast<T>(static_cast<Underlying>(numeric));
        }
    }
}

} // namespace Detail
} // namespace Miro
