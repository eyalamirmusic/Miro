#pragma once

#include <array>
#include <string_view>

namespace Miro::Detail
{

// Extracts a compile-time, compiler-derived name for type T from
// __PRETTY_FUNCTION__ / __FUNCSIG__. Same trick as enumNameRaw in
// ReflectEnum.h, generalized to any T. Returns the spelling the
// compiler chose for the template argument (may include namespace
// qualifiers and template arguments).
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

// Like rawTypeNameOf but strips the leading C++ class-key keyword that
// MSVC's __FUNCSIG__ emits ("struct ", "class ", "enum "). All namespace
// qualifiers and template arguments are preserved. This is the canonical
// spelling to compare or hash across compilers — Clang/GCC don't emit
// the keyword in __PRETTY_FUNCTION__, so this is a no-op there.
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

// Returns just the unqualified short name. Strips namespaces (incl.
// "(anonymous namespace)::"). Templates and STL types are deliberately
// left intact in the raw form — callers that want to filter them can
// inspect for '<' / "std::" themselves.
template <typename T>
constexpr std::string_view typeNameOf()
{
    auto name = qualifiedNameOf<T>();

    // Find the rightmost "::" that is *not* inside template angle
    // brackets, and trim everything before it.
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

// True if T is a "named" user-defined type — i.e. not a template
// instantiation and not in the std namespace. Used by the TypeScript
// exporter to decide whether to register T as a reusable type or
// inline its shape.
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
