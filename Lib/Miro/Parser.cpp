#include "Json.h"

#include <charconv>

namespace Miro::Json
{

class Parser
{
public:
    explicit Parser(std::string_view inputToUse)
        : input(inputToUse)
        , position(0)
    {
    }

    Value parseValue()
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

    bool atEnd() const { return position >= input.size(); }

    void skipWhitespace()
    {
        while (!atEnd()
               && (current() == ' ' || current() == '\t' || current() == '\n'
                   || current() == '\r'))
        {
            advance();
        }
    }

    [[noreturn]] void error(const std::string& messageToUse) const
    {
        throw ParseError("JSON parse error at position " + std::to_string(position)
                         + ": " + messageToUse);
    }

private:
    Value parseNull()
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

    Value parseBool()
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

    Value parseNumber()
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
        auto [ptr, ec] =
            std::from_chars(text.data(), text.data() + text.size(), value);

        if (ec != std::errc {})
        {
            error("failed to parse number");
        }

        return {value};
    }

    Value parseString() { return {parseStringRaw()}; }

    std::string parseStringRaw()
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
                                error("unexpected end of "
                                      "unicode escape");
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

    Value parseArray()
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

    Value parseObject()
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

    char current() const { return input[position]; }

    char advance() { return input[position++]; }

    void expect(char charToUse)
    {
        if (atEnd() || current() != charToUse)
        {
            error("expected '" + std::string(1, charToUse) + "'"
                  + (atEnd() ? " but reached end of input"
                             : " but got '" + std::string(1, current()) + "'"));
        }
        advance();
    }

    std::string_view input;
    std::size_t position;
};

Value parse(std::string_view inputToUse)
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

} // namespace Miro::Json
