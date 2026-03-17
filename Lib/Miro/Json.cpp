#include "Json.h"

#include <charconv>

namespace Miro::Json
{

// --- Value ---

Value::Value()
    : data(nullptr)
{
}

Value::Value(Null /*valueToUse*/)
    : data(nullptr)
{
}

Value::Value(bool valueToUse)
    : data(valueToUse)
{
}

Value::Value(double valueToUse)
    : data(valueToUse)
{
}

Value::Value(int valueToUse)
    : data(static_cast<double>(valueToUse))
{
}

Value::Value(std::string valueToUse)
    : data(std::move(valueToUse))
{
}

Value::Value(const char* valueToUse)
    : data(std::string(valueToUse))
{
}

Value::Value(Array valueToUse)
    : data(std::move(valueToUse))
{
}

Value::Value(Object valueToUse)
    : data(std::move(valueToUse))
{
}

bool Value::isNull() const
{
    return std::holds_alternative<Null>(data);
}

bool Value::isBool() const
{
    return std::holds_alternative<bool>(data);
}

bool Value::isNumber() const
{
    return std::holds_alternative<double>(data);
}

bool Value::isString() const
{
    return std::holds_alternative<std::string>(data);
}

bool Value::isArray() const
{
    return std::holds_alternative<Array>(data);
}

bool Value::isObject() const
{
    return std::holds_alternative<Object>(data);
}

bool Value::asBool() const
{
    return std::get<bool>(data);
}

double Value::asNumber() const
{
    return std::get<double>(data);
}

const std::string& Value::asString() const
{
    return std::get<std::string>(data);
}

const Array& Value::asArray() const
{
    return std::get<Array>(data);
}

const Object& Value::asObject() const
{
    return std::get<Object>(data);
}

Value& Value::operator[](const std::string& keyToUse)
{
    return std::get<Object>(data).at(keyToUse);
}

const Value& Value::operator[](const std::string& keyToUse) const
{
    return std::get<Object>(data).at(keyToUse);
}

Value& Value::operator[](std::size_t indexToUse)
{
    return std::get<Array>(data).at(indexToUse);
}

const Value& Value::operator[](std::size_t indexToUse) const
{
    return std::get<Array>(data).at(indexToUse);
}

// --- Parser ---

Value Parser::parse(std::string_view inputToUse)
{
    auto parser = Parser(inputToUse);
    auto result = parser.parseValue();
    parser.skipWhitespace();

    if (!parser.atEnd())
    {
        parser.error("unexpected trailing content");
    }

    return result;
}

Parser::Parser(std::string_view inputToUse)
    : input(inputToUse)
    , position(0)
{
}

Value Parser::parseValue()
{
    skipWhitespace();

    if (atEnd())
        error("unexpected end of input");

    auto c = current();

    if (c == 'n')
        return parseNull();
    if (c == 't' || c == 'f')
        return parseBool();
    if (c == '"')
        return parseString();
    if (c == '[')
        return parseArray();
    if (c == '{')
        return parseObject();
    if (c == '-' || (c >= '0' && c <= '9'))
        return parseNumber();

    error("unexpected character '" + std::string(1, c) + "'");
}

Value Parser::parseNull()
{
    auto keyword = std::string_view("null");
    auto remaining = input.substr(position);

    if (!remaining.starts_with(keyword))
    {
        error("expected 'null'");
    }

    position += keyword.size();

    return {nullptr};
}

Value Parser::parseBool()
{
    auto trueKeyword = std::string_view("true");
    auto falseKeyword = std::string_view("false");
    auto remaining = input.substr(position);

    if (remaining.starts_with(trueKeyword))
    {
        position += trueKeyword.size();
        return {true};
    }

    if (remaining.starts_with(falseKeyword))
    {
        position += falseKeyword.size();
        return {false};
    }

    error("expected 'true' or 'false'");
}

Value Parser::parseNumber()
{
    auto start = position;

    if (current() == '-')
    {
        advance();
    }

    if (atEnd())
    {
        error("unexpected end of number");
    }

    if (current() == '0')
    {
        advance();
    }
    else if (current() >= '1' && current() <= '9')
    {
        while (!atEnd() && current() >= '0' && current() <= '9')
        {
            advance();
        }
    }
    else
    {
        error("invalid number");
    }

    if (!atEnd() && current() == '.')
    {
        advance();
        if (atEnd() || current() < '0' || current() > '9')
        {
            error("expected digit after decimal point");
        }
        while (!atEnd() && current() >= '0' && current() <= '9')
        {
            advance();
        }
    }

    if (!atEnd() && (current() == 'e' || current() == 'E'))
    {
        advance();
        if (!atEnd() && (current() == '+' || current() == '-'))
        {
            advance();
        }
        if (atEnd() || current() < '0' || current() > '9')
        {
            error("expected digit in exponent");
        }
        while (!atEnd() && current() >= '0' && current() <= '9')
        {
            advance();
        }
    }

    auto text = input.substr(start, position - start);
    auto value = double {};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);

    if (ec != std::errc {})
    {
        error("failed to parse number");
    }

    return {value};
}

Value Parser::parseString()
{
    return {parseStringRaw()};
}

std::string Parser::parseStringRaw()
{
    expect('"');
    auto result = std::string {};

    while (!atEnd() && current() != '"')
    {
        if (current() == '\\')
        {
            advance();
            if (atEnd())
            {
                error("unexpected end of string escape");
            }

            auto escaped = current();
            advance();

            switch (escaped)
            {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'u':
                {
                    auto hex = std::string {};
                    for (auto i = 0; i < 4; ++i)
                    {
                        if (atEnd())
                        {
                            error("unexpected end of unicode "
                                  "escape");
                        }
                        hex += current();
                        advance();
                    }

                    auto codepoint = unsigned {};
                    auto [ptr, ec] = std::from_chars(
                        hex.data(), hex.data() + hex.size(), codepoint, 16);

                    if (ec != std::errc {})
                    {
                        error("invalid unicode escape");
                    }

                    if (codepoint <= 0x7F)
                    {
                        result += static_cast<char>(codepoint);
                    }
                    else if (codepoint <= 0x7FF)
                    {
                        result += static_cast<char>(0xC0 | (codepoint >> 6));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else
                    {
                        result += static_cast<char>(0xE0 | (codepoint >> 12));
                        result +=
                            static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    break;
                }
                default:
                    error("invalid escape character '" + std::string(1, escaped)
                          + "'");
            }
        }
        else
        {
            result += current();
            advance();
        }
    }

    expect('"');
    return result;
}

Value Parser::parseArray()
{
    expect('[');
    skipWhitespace();

    auto elements = Array {};

    if (!atEnd() && current() != ']')
    {
        elements.push_back(parseValue());
        skipWhitespace();

        while (!atEnd() && current() == ',')
        {
            advance();
            elements.push_back(parseValue());
            skipWhitespace();
        }
    }

    expect(']');
    return Value(std::move(elements));
}

Value Parser::parseObject()
{
    expect('{');
    skipWhitespace();

    auto entries = Object {};

    if (!atEnd() && current() != '}')
    {
        skipWhitespace();
        auto key = parseStringRaw();
        skipWhitespace();
        expect(':');
        auto value = parseValue();
        entries.emplace(std::move(key), std::move(value));
        skipWhitespace();

        while (!atEnd() && current() == ',')
        {
            advance();
            skipWhitespace();
            key = parseStringRaw();
            skipWhitespace();
            expect(':');
            value = parseValue();
            entries.emplace(std::move(key), std::move(value));
            skipWhitespace();
        }
    }

    expect('}');
    return Value(std::move(entries));
}

char Parser::current() const
{
    return input[position];
}

char Parser::advance()
{
    return input[position++];
}

bool Parser::atEnd() const
{
    return position >= input.size();
}

void Parser::skipWhitespace()
{
    while (!atEnd()
           && (current() == ' ' || current() == '\t' || current() == '\n'
               || current() == '\r'))
    {
        advance();
    }
}

void Parser::expect(char charToUse)
{
    if (atEnd() || current() != charToUse)
    {
        error("expected '" + std::string(1, charToUse) + "'"
              + (atEnd() ? " but reached end of input"
                         : " but got '" + std::string(1, current()) + "'"));
    }
    advance();
}

void Parser::error(const std::string& messageToUse) const
{
    throw ParseError("JSON parse error at position " + std::to_string(position)
                     + ": " + messageToUse);
}

} // namespace Miro::Json
