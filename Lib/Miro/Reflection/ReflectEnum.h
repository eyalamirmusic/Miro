#pragma once

#include "Reflector.h"
#include "TypeName.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Miro
{

// Specialize to change the enumerator range probed for an enum type.
// An unscoped enum with no fixed underlying type must not be probed
// outside its enumerators (UB) — give it a base or use `enum class`.
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

    // Rightmost "::" at paren depth 0, so the one inside an "(anonymous
    // namespace)::Foo" qualifier is skipped — which also leaves a cast
    // spelling like "((Ns)::Foo)42" with the leading '(' rejected below.
    auto depth = 0;
    auto lastColon = std::string_view::npos;

    for (auto i = std::size_t {0}; i + 1 < name.size(); ++i)
    {
        if (name[i] == '(')
            ++depth;
        else if (name[i] == ')' && depth > 0)
            --depth;
        else if (depth == 0 && name[i] == ':' && name[i + 1] == ':')
            lastColon = i;
    }

    if (lastColon != std::string_view::npos)
        name.remove_prefix(lastColon + 2);

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

template <typename E>
    requires std::is_enum_v<E>
Vector<std::string_view> enumNames()
{
    auto names = Vector<std::string_view> {};

    for (auto& [_, name]: Detail::enumTable<E>)
        if (!name.empty())
            names.add(name);

    return names;
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
            ref.visitEnum(TypeId {typeNameOf<T>(), qualifiedNameOf<T>()},
                          enumNames<T>());
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
