#pragma once

#include "../Detail/StringUtilities.h"

#include <string>
#include <string_view>

// Swift-codegen naming helpers shared by the type and client renderers.
// Internal to the Swift backend — not pulled into <Miro/Miro.h>.
//
// Lives in Miro::Swift::Naming (not ::Detail) so the renderers can still
// reach Miro::Detail's string utilities via an unqualified `Detail::`
// without the two namespaces colliding.

namespace Miro::Swift::Naming
{

// True for the reserved words that cannot be used as a Swift identifier
// without backticks.
inline bool isSwiftKeyword(std::string_view name)
{
    static constexpr std::string_view keywords[] = {
        "associatedtype",
        "class",
        "deinit",
        "enum",
        "extension",
        "fileprivate",
        "func",
        "import",
        "init",
        "inout",
        "internal",
        "let",
        "open",
        "operator",
        "private",
        "protocol",
        "public",
        "rethrows",
        "static",
        "struct",
        "subscript",
        "typealias",
        "var",
        "break",
        "case",
        "continue",
        "default",
        "defer",
        "do",
        "else",
        "fallthrough",
        "for",
        "guard",
        "if",
        "in",
        "repeat",
        "return",
        "switch",
        "where",
        "while",
        "as",
        "Any",
        "catch",
        "false",
        "is",
        "nil",
        "super",
        "self",
        "Self",
        "throw",
        "throws",
        "true",
        "try",
    };

    for (auto& keyword: keywords)
        if (name == keyword)
            return true;

    return false;
}

// Renders `name` as a Swift declaration identifier: bare when it is a
// valid, non-keyword identifier; backtick-escaped when it is a keyword
// (`default` -> `` `default` ``). Names that aren't valid Swift
// identifiers at all are returned unchanged — callers that can hit those
// (struct fields, from arbitrary JSON keys) sanitize separately and add
// a CodingKeys mapping.
inline std::string swiftIdentifier(std::string_view name)
{
    if (Detail::isAsciiIdentifier(name) && isSwiftKeyword(name))
        return "`" + std::string {name} + "`";

    return std::string {name};
}

// Escapes a string for embedding inside a Swift "..." literal.
inline std::string escapeSwiftString(std::string_view value)
{
    auto out = std::string {};

    for (auto c: value)
    {
        if (c == '\\' || c == '"')
            out += '\\';
        out += c;
    }

    return out;
}

} // namespace Miro::Swift::Naming
