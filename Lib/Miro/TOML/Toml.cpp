#include "Toml.h"

// toml++ asserts on some malformed input in debug builds (a lone "[" at
// the end of a document) where a release build reports a parse error.
// Malformed input is the caller's data, not a bug, so it always reports.
#define TOML_ASSERT(expression) static_cast<void>(0)
#include <toml++/toml.hpp>

#include <cmath>
#include <utility>

namespace Miro::Toml
{
namespace
{

std::string describeParseFailure(const std::string& description,
                                 const std::string& keyPath,
                                 std::uint32_t line,
                                 std::uint32_t column)
{
    auto message = "TOML parse error at line " + std::to_string(line) + ", column "
                   + std::to_string(column);

    if (!keyPath.empty())
        message += ", key '" + keyPath + "'";

    return message + ": " + description;
}

std::string describePrintFailure(const std::string& description,
                                 const std::string& keyPath)
{
    if (keyPath.empty())
        return "TOML print error: " + description;

    return "TOML print error at key '" + keyPath + "': " + description;
}

std::string appendKeyToPath(const std::string& path, std::string_view key)
{
    if (path.empty())
        return std::string {key};

    return path + "." + std::string {key};
}

std::string appendIndexToPath(const std::string& path, std::size_t index)
{
    return path + "[" + std::to_string(index) + "]";
}

[[noreturn]] void throwUnsupportedNode(const toml::node& node,
                                       const std::string& path,
                                       std::string_view typeName)
{
    auto description = "TOML " + std::string {typeName}
                       + " values are not supported, because Miro has no "
                         "date or time type to load them into";
    auto& begin = node.source().begin;
    throw ParseError {description, path, begin.line, begin.column};
}

Json::Value convertNodeToJson(const toml::node& node, const std::string& path);

Json::Value convertTableToJson(const toml::table& table, const std::string& path)
{
    auto object = Json::Object {};

    for (auto& [key, child]: table)
    {
        auto childPath = appendKeyToPath(path, key.str());
        object.emplace(std::string {key.str()}, convertNodeToJson(child, childPath));
    }

    return Json::Value {std::move(object)};
}

Json::Value convertArrayToJson(const toml::array& array, const std::string& path)
{
    auto elements = Json::Array {};

    for (auto index = std::size_t {0}; index < array.size(); ++index)
        elements.add(
            convertNodeToJson(array[index], appendIndexToPath(path, index)));

    return Json::Value {std::move(elements)};
}

Json::Value convertNodeToJson(const toml::node& node, const std::string& path)
{
    switch (node.type())
    {
        case toml::node_type::table:
            return convertTableToJson(*node.as_table(), path);
        case toml::node_type::array:
            return convertArrayToJson(*node.as_array(), path);
        case toml::node_type::string:
            return Json::Value {node.as_string()->get()};
        case toml::node_type::integer:
            return Json::Value {node.as_integer()->get()};
        case toml::node_type::floating_point:
            return Json::Value {node.as_floating_point()->get()};
        case toml::node_type::boolean:
            return Json::Value {node.as_boolean()->get()};
        case toml::node_type::date:
            throwUnsupportedNode(node, path, "date");
        case toml::node_type::time:
            throwUnsupportedNode(node, path, "time");
        case toml::node_type::date_time:
            throwUnsupportedNode(node, path, "date-time");
        case toml::node_type::none:
            break;
    }

    throwUnsupportedNode(node, path, "untyped");
}

bool isBareKeyCharacter(char character)
{
    auto isLower = character >= 'a' && character <= 'z';
    auto isUpper = character >= 'A' && character <= 'Z';
    auto isDigit = character >= '0' && character <= '9';
    return isLower || isUpper || isDigit || character == '_' || character == '-';
}

bool isBareKey(std::string_view key)
{
    if (key.empty())
        return false;

    for (auto character: key)
    {
        if (!isBareKeyCharacter(character))
            return false;
    }

    return true;
}

void appendHexEscape(std::string& output, unsigned char character)
{
    constexpr auto hexDigits = std::string_view {"0123456789ABCDEF"};
    constexpr auto nibbleBits = 4;
    constexpr auto nibbleMask = 0x0F;

    output += "\\u00";
    output += hexDigits[static_cast<std::size_t>(character >> nibbleBits)];
    output += hexDigits[static_cast<std::size_t>(character & nibbleMask)];
}

bool isControlCharacter(unsigned char character)
{
    constexpr auto firstPrintable = 0x20;
    constexpr auto deleteCharacter = 0x7F;
    return character < firstPrintable || character == deleteCharacter;
}

void appendQuotedString(std::string& output, std::string_view text)
{
    output += '"';

    for (auto character: text)
    {
        auto byte = static_cast<unsigned char>(character);

        if (character == '"')
            output += "\\\"";
        else if (character == '\\')
            output += "\\\\";
        else if (character == '\b')
            output += "\\b";
        else if (character == '\t')
            output += "\\t";
        else if (character == '\n')
            output += "\\n";
        else if (character == '\f')
            output += "\\f";
        else if (character == '\r')
            output += "\\r";
        else if (isControlCharacter(byte))
            appendHexEscape(output, byte);
        else
            output += character;
    }

    output += '"';
}

void appendKey(std::string& output, std::string_view key)
{
    if (isBareKey(key))
        output += key;
    else
        appendQuotedString(output, key);
}

bool hasFractionOrExponent(std::string_view number)
{
    return number.find_first_of(".eE") != std::string_view::npos;
}

// TOML spells the non-finite floats as words, and a finite float must
// carry a '.' or an exponent or it reads back as an integer.
void appendFloat(std::string& output, double number)
{
    if (std::isnan(number))
    {
        output += "nan";
        return;
    }

    if (std::isinf(number))
    {
        output += number < 0 ? "-inf" : "inf";
        return;
    }

    auto text = Json::print(Json::Value {number});
    output += text;

    if (!hasFractionOrExponent(text))
        output += ".0";
}

void appendInlineValue(std::string& output,
                       const Json::Value& value,
                       const std::string& path);

void appendInlineArray(std::string& output,
                       const Json::Array& array,
                       const std::string& path)
{
    auto& elements = array.getVector();

    if (elements.empty())
    {
        output += "[]";
        return;
    }

    output += "[ ";

    for (auto index = std::size_t {0}; index < elements.size(); ++index)
    {
        if (index > 0)
            output += ", ";

        appendInlineValue(output, elements[index], appendIndexToPath(path, index));
    }

    output += " ]";
}

void appendInlineTable(std::string& output,
                       const Json::Object& object,
                       const std::string& path)
{
    auto isFirst = true;
    output += "{";

    for (auto& [key, value]: object)
    {
        if (value.isNull())
            continue;

        output += isFirst ? " " : ", ";
        isFirst = false;
        appendKey(output, key);
        output += " = ";
        appendInlineValue(output, value, appendKeyToPath(path, key));
    }

    output += isFirst ? "}" : " }";
}

void appendInlineValue(std::string& output,
                       const Json::Value& value,
                       const std::string& path)
{
    std::visit(
        [&](auto& alternative)
        {
            using Alternative = std::decay_t<decltype(alternative)>;

            if constexpr (std::same_as<Alternative, Json::Null>)
                throw PrintError {"TOML has no null, so an array element "
                                  "cannot be null",
                                  path};
            else if constexpr (std::same_as<Alternative, bool>)
                output += alternative ? "true" : "false";
            else if constexpr (std::same_as<Alternative, std::int64_t>)
                output += std::to_string(alternative);
            else if constexpr (std::same_as<Alternative, double>)
                appendFloat(output, alternative);
            else if constexpr (std::same_as<Alternative, std::string>)
                appendQuotedString(output, alternative);
            else if constexpr (std::same_as<Alternative, Json::Array>)
                appendInlineArray(output, alternative, path);
            else
                appendInlineTable(output, alternative, path);
        },
        value.data);
}

bool isArrayOfTables(const Json::Value& value)
{
    if (!value.isArray())
        return false;

    auto& elements = value.asArray().getVector();

    if (elements.empty())
        return false;

    for (auto& element: elements)
    {
        if (!element.isObject())
            return false;
    }

    return true;
}

bool isWrittenUnderAHeader(const Json::Value& value)
{
    return value.isObject() || isArrayOfTables(value);
}

struct TablePosition
{
    std::string header;
    std::string path;
};

TablePosition descendInto(const TablePosition& parent, std::string_view key)
{
    auto header = parent.header;

    if (!header.empty())
        header += '.';

    appendKey(header, key);
    return {header, appendKeyToPath(parent.path, key)};
}

void appendSectionBreak(std::string& output)
{
    if (!output.empty())
        output += '\n';
}

void appendTableBody(std::string& output,
                     const Json::Object& object,
                     const TablePosition& position)
{
    for (auto& [key, value]: object)
    {
        if (value.isNull() || isWrittenUnderAHeader(value))
            continue;

        appendKey(output, key);
        output += " = ";
        appendInlineValue(output, value, appendKeyToPath(position.path, key));
        output += '\n';
    }

    for (auto& [key, value]: object)
    {
        if (!value.isObject())
            continue;

        auto child = descendInto(position, key);
        appendSectionBreak(output);
        output += "[" + child.header + "]\n";
        appendTableBody(output, value.asObject(), child);
    }

    for (auto& [key, value]: object)
    {
        if (!isArrayOfTables(value))
            continue;

        auto child = descendInto(position, key);
        auto& elements = value.asArray().getVector();

        for (auto index = std::size_t {0}; index < elements.size(); ++index)
        {
            appendSectionBreak(output);
            output += "[[" + child.header + "]]\n";
            appendTableBody(output,
                            elements[index].asObject(),
                            {child.header, appendIndexToPath(child.path, index)});
        }
    }
}

} // namespace

ParseError::ParseError(const std::string& descriptionToUse,
                       std::string keyPathToUse,
                       std::uint32_t lineToUse,
                       std::uint32_t columnToUse)
    : Json::Error(describeParseFailure(
          descriptionToUse, keyPathToUse, lineToUse, columnToUse))
    , keyPath(std::move(keyPathToUse))
    , line(lineToUse)
    , column(columnToUse)
{
}

PrintError::PrintError(const std::string& descriptionToUse, std::string keyPathToUse)
    : Json::Error(describePrintFailure(descriptionToUse, keyPathToUse))
    , keyPath(std::move(keyPathToUse))
{
}

Json::Value parse(std::string_view inputToUse)
{
    try
    {
        auto table = toml::parse(inputToUse);
        return convertTableToJson(table, {});
    }
    catch (const toml::parse_error& error)
    {
        auto& begin = error.source().begin;
        throw ParseError {
            std::string {error.description()}, {}, begin.line, begin.column};
    }
}

std::string print(const Json::Value& valueToUse)
{
    if (!valueToUse.isObject())
        throw PrintError {"a TOML document must be a table, so the value to "
                          "print must be an object",
                          {}};

    auto output = std::string {};
    appendTableBody(output, valueToUse.asObject(), {});
    return output;
}

} // namespace Miro::Toml
