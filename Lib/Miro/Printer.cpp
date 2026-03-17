#include "Json.h"

#include <array>
#include <cmath>
#include <sstream>

namespace Miro::Json
{

void printTo(std::string& output, const Value& value);

void printString(std::string& output, const std::string& str)
{
    output += '"';

    for (auto c: str)
    {
        switch (c)
        {
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    auto buf = std::array<char, 8> {};
                    std::snprintf(
                        buf.data(),
                        buf.size(),
                        "\\u%04x",
                        static_cast<unsigned>(static_cast<unsigned char>(c)));
                    output += buf.data();
                }
                else
                {
                    output += c;
                }
        }
    }

    output += '"';
}

void printNumber(std::string& output, double number)
{
    if (std::isfinite(number) && number == std::floor(number)
        && std::abs(number) < 1e15)
    {
        auto stream = std::ostringstream {};
        stream << static_cast<long long>(number);
        output += stream.str();
    }
    else
    {
        auto stream = std::ostringstream {};
        stream << number;
        output += stream.str();
    }
}

void printArray(std::string& output, const Array& array)
{
    output += '[';

    for (auto i = std::size_t {0}; i < array.size(); ++i)
    {
        if (i > 0)
            output += ',';

        printTo(output, array[i]);
    }

    output += ']';
}

void printObject(std::string& output, const Object& object)
{
    output += '{';

    auto first = true;

    for (const auto& [key, value]: object)
    {
        if (!first)
            output += ',';

        first = false;
        printString(output, key);
        output += ':';
        printTo(output, value);
    }

    output += '}';
}

void printTo(std::string& output, const Value& value)
{
    if (value.isNull())
    {
        output += "null";
    }
    else if (value.isBool())
    {
        output += value.asBool() ? "true" : "false";
    }
    else if (value.isNumber())
    {
        printNumber(output, value.asNumber());
    }
    else if (value.isString())
    {
        printString(output, value.asString());
    }
    else if (value.isArray())
    {
        printArray(output, value.asArray());
    }
    else if (value.isObject())
    {
        printObject(output, value.asObject());
    }
}

std::string print(const Value& valueToUse)
{
    auto result = std::string {};
    printTo(result, valueToUse);
    return result;
}

} // namespace Miro::Json
