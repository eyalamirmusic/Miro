#pragma once

#include <array>
#include <string_view>

namespace Miro::Detail
{

// Whatever spelling the compiler chose, including namespace qualifiers,
// template arguments and (on MSVC) a leading class-key keyword.
template <typename T>
constexpr std::string_view rawTypeNameOf()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr auto prefix = std::string_view {"T = "};
    constexpr auto terminators = std::string_view {";]"};
    auto sig = std::string_view {__PRETTY_FUNCTION__};
    auto start = sig.find(prefix) + prefix.size();
    auto end = sig.find_first_of(terminators, start);
    return sig.substr(start, end - start);
#elif defined(_MSC_VER)
    constexpr auto prefix = std::string_view {"rawTypeNameOf<"};
    constexpr auto terminator = std::string_view {">("};
    auto sig = std::string_view {__FUNCSIG__};
    auto start = sig.find(prefix) + prefix.size();
    auto end = sig.find(terminator, start);
    return sig.substr(start, end - start);
#else
    return {};
#endif
}

template <typename T>
constexpr std::string_view qualifiedNameOf()
{
    auto raw = rawTypeNameOf<T>();
    constexpr auto prefixes = std::array {
        std::string_view {"struct "},
        std::string_view {"class "},
        std::string_view {"enum "},
    };
    for (auto p: prefixes)
        if (raw.starts_with(p))
            return raw.substr(p.size());
    return raw;
}

template <typename T>
constexpr std::string_view typeNameOf()
{
    auto name = qualifiedNameOf<T>();

    // Rightmost "::" outside template angle brackets.
    auto depth = 0;
    auto lastColon = std::string_view::npos;

    for (auto i = std::size_t {0}; i + 1 < name.size(); ++i)
    {
        if (name[i] == '<')
            ++depth;
        else if (name[i] == '>' && depth > 0)
            --depth;
        else if (depth == 0 && name[i] == ':' && name[i + 1] == ':')
            lastColon = i;
    }

    if (lastColon != std::string_view::npos)
        name.remove_prefix(lastColon + 2);

    return name;
}

template <auto Member>
constexpr std::string_view memberNameOf()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr auto prefix = std::string_view {"Member = "};
    constexpr auto terminators = std::string_view {";]"};
    auto sig = std::string_view {__PRETTY_FUNCTION__};
    auto start = sig.find(prefix) + prefix.size();
    auto end = sig.find_first_of(terminators, start);
#elif defined(_MSC_VER)
    constexpr auto prefix = std::string_view {"memberNameOf<"};
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

    if (!name.empty() && name.front() == '&')
        name.remove_prefix(1);

    // Rightmost "::" outside template angle brackets and outside the
    // parens of an "(anonymous namespace)" qualifier.
    auto angleDepth = 0;
    auto parenDepth = 0;
    auto lastColon = std::string_view::npos;

    for (auto i = std::size_t {0}; i + 1 < name.size(); ++i)
    {
        auto c = name[i];
        if (c == '<')
            ++angleDepth;
        else if (c == '>' && angleDepth > 0)
            --angleDepth;
        else if (c == '(')
            ++parenDepth;
        else if (c == ')' && parenDepth > 0)
            --parenDepth;
        else if (angleDepth == 0 && parenDepth == 0 && c == ':'
                 && name[i + 1] == ':')
            lastColon = i;
    }

    if (lastColon != std::string_view::npos)
        name.remove_prefix(lastColon + 2);

    // MSVC renders a pmf NTTP as a full signature, so drop the trailing
    // "(params) cv" that GCC/Clang never emit.
    if (auto paren = name.find('('); paren != std::string_view::npos)
        name = name.substr(0, paren);

    return name;
}

template <typename T>
constexpr bool isNamedUserType()
{
    auto raw = rawTypeNameOf<T>();

    if (raw.find('<') != std::string_view::npos)
        return false;
    if (raw.find("std::") != std::string_view::npos)
        return false;
    if (raw.find("(anonymous") != std::string_view::npos)
        return true; // anonymous-namespace user types are still user types
    return true;
}

} // namespace Miro::Detail
