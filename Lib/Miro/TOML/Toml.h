#pragma once

#include "../JSON/Json.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

// TOML is read into, and written from, the same Json::Value the JSON
// layer uses: a TOML table is an Object, an array an Array, and every
// scalar the matching alternative. The reflection layer then serializes
// structs through the existing JsonReflector, so a type that reflects
// to JSON reflects to TOML with no extra code. toml++ does the parsing
// and the printing and never appears in this header.

namespace Miro::Toml
{

// Thrown by parse() for malformed TOML and for TOML a Json::Value can't
// hold (a date or time). The line and column are 1-based positions in
// the document; keyPath names the offending key ("deps.fmt.version",
// "targets[2].name") when there is one, and is empty for a syntax error.
class ParseError : public Json::Error
{
public:
    ParseError(const std::string& descriptionToUse,
               std::string keyPathToUse,
               std::uint32_t lineToUse,
               std::uint32_t columnToUse);

    std::string keyPath;
    std::uint32_t line = 0;
    std::uint32_t column = 0;
};

// Thrown by print() for a value TOML can't express: a root that isn't
// an object, or a null inside an array. keyPath names where it is.
class PrintError : public Json::Error
{
public:
    PrintError(const std::string& descriptionToUse, std::string keyPathToUse);

    std::string keyPath;
};

Json::Value parse(std::string_view inputToUse);

// Writes `valueToUse`, which must be an Object, as a TOML document. A
// null member of an object is left out, since TOML has no null and a
// missing key loads back as one. An array whose every element is an
// object becomes an [[array of tables]].
std::string print(const Json::Value& valueToUse);

} // namespace Miro::Toml
