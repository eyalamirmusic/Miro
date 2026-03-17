#include "Json.h"

#include <array>
#include <charconv>

namespace Miro::Json
{

class Parser
{
public:
    explicit Parser(std::string_view inputToUse)
        : input(inputToUse.data())
        , end(inputToUse.data() + inputToUse.size())
        , pos(inputToUse.data())
    {
    }

    Value parseValue()
    {
        skipWhitespace();

        if (atEnd())
            error("unexpected end of input");

        auto c = *pos;

        if (c == '"')
            return parseString();
        if (c == '{')
            return parseObject();
        if (c == '[')
            return parseArray();
        if (c == 't' || c == 'f')
            return parseBool();
        if (c == 'n')
            return parseNull();
        if (c == '-' || (c >= '0' && c <= '9'))
            return parseNumber();

        error("unexpected character '" + std::string(1, c) + "'");
    }

    bool atEnd() const { return pos >= end; }

    void skipWhitespace()
    {
        while (pos < end
               && (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r'))
        {
            ++pos;
        }
    }

    [[noreturn]] void error(const std::string& messageToUse) const
    {
        throw ParseError("JSON parse error at position "
                         + std::to_string(pos - input) + ": " + messageToUse);
    }

private:
    // --- Primitives ---

    Value parseNull()
    {
        expectKeyword("null", 4);
        return {nullptr};
    }

    Value parseBool()
    {
        if (remaining() >= 4 && pos[0] == 't' && pos[1] == 'r' && pos[2] == 'u'
            && pos[3] == 'e')
        {
            pos += 4;
            return {true};
        }

        expectKeyword("false", 5);
        return {false};
    }

    Value parseNumber()
    {
        auto start = pos;

        if (*pos == '-')
            ++pos;

        if (pos >= end)
            error("unexpected end of number");

        if (*pos == '0')
            ++pos;
        else if (*pos >= '1' && *pos <= '9')
            skipDigits();
        else
            error("invalid number");

        if (pos < end && *pos == '.')
        {
            ++pos;
            if (pos >= end || *pos < '0' || *pos > '9')
                error("expected digit after decimal point");
            skipDigits();
        }

        if (pos < end && (*pos == 'e' || *pos == 'E'))
        {
            ++pos;
            if (pos < end && (*pos == '+' || *pos == '-'))
                ++pos;
            if (pos >= end || *pos < '0' || *pos > '9')
                error("expected digit in exponent");
            skipDigits();
        }

        auto value = double {};
        auto [ptr, ec] = std::from_chars(start, pos, value);
        if (ec != std::errc {})
            error("failed to parse number");

        return {value};
    }

    // --- Strings ---

    Value parseString() { return {parseStringRaw()}; }

    std::string parseStringRaw()
    {
        expect('"');

        auto start = pos;
        skipToStringBreak();

        if (pos < end && *pos == '"')
        {
            auto result = std::string(start, pos - start);
            ++pos;
            return result;
        }

        auto result = std::string(start, pos - start);
        parseStringEscapes(result);
        return result;
    }

    void parseStringEscapes(std::string& result)
    {
        while (pos < end && *pos != '"')
        {
            if (*pos == '\\')
            {
                parseEscapeSequence(result);
            }
            else
            {
                auto start = pos;
                skipToStringBreak();
                result.append(start, pos - start);
            }
        }

        if (pos >= end)
            error("expected '\"' but reached end of "
                  "input");

        ++pos;
    }

    void parseEscapeSequence(std::string& result)
    {
        ++pos;

        if (pos >= end)
            error("unexpected end of string escape");

        auto escaped = *pos;
        ++pos;

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
                parseUnicodeEscape(result);
                break;
            default:
                error("invalid escape character '" + std::string(1, escaped) + "'");
        }
    }

    void parseUnicodeEscape(std::string& result)
    {
        if (remaining() < 4)
            error("unexpected end of unicode escape");

        auto hex = std::array<char, 4> {pos[0], pos[1], pos[2], pos[3]};
        pos += 4;

        auto codepoint = unsigned {};
        auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + 4, codepoint, 16);

        if (ec != std::errc {})
            error("invalid unicode escape");

        appendUtf8(result, codepoint);
    }

    // --- Containers ---

    Value parseArray()
    {
        expect('[');
        skipWhitespace();

        auto elements = Array {};

        if (pos < end && *pos != ']')
        {
            elements.reserve(16);
            elements.push_back(parseValue());
            skipWhitespace();

            while (pos < end && *pos == ',')
            {
                ++pos;
                elements.push_back(parseValue());
                skipWhitespace();
            }
        }

        expect(']');
        return {std::move(elements)};
    }

    Value parseObject()
    {
        expect('{');
        skipWhitespace();

        auto entries = Object {};

        if (pos < end && *pos != '}')
        {
            parseKeyValueInto(entries);

            while (pos < end && *pos == ',')
            {
                ++pos;
                parseKeyValueInto(entries);
            }
        }

        expect('}');
        return {std::move(entries)};
    }

    void parseKeyValueInto(Object& entries)
    {
        skipWhitespace();
        auto key = parseStringRaw();
        skipWhitespace();
        expect(':');
        auto value = parseValue();
        entries.emplace(std::move(key), std::move(value));
        skipWhitespace();
    }

    // --- Helpers ---

    static void appendUtf8(std::string& result, unsigned codepoint)
    {
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
            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    void skipDigits()
    {
        while (pos < end && *pos >= '0' && *pos <= '9')
            ++pos;
    }

    void skipToStringBreak()
    {
        while (pos < end && *pos != '"' && *pos != '\\')
            ++pos;
    }

    std::ptrdiff_t remaining() const { return end - pos; }

    void expectKeyword(const char* keywordToUse, int lengthToUse)
    {
        if (remaining() < lengthToUse)
            error("expected '" + std::string(keywordToUse) + "'");

        for (auto i = 0; i < lengthToUse; ++i)
        {
            if (pos[i] != keywordToUse[i])
                error("expected '" + std::string(keywordToUse) + "'");
        }

        pos += lengthToUse;
    }

    void expect(char charToUse)
    {
        if (pos >= end || *pos != charToUse)
        {
            error("expected '" + std::string(1, charToUse) + "'"
                  + (pos >= end ? " but reached end of input"
                                : " but got '" + std::string(1, *pos) + "'"));
        }
        ++pos;
    }

    const char* input;
    const char* end;
    const char* pos;
};

Value parse(std::string_view inputToUse)
{
    auto parser = Parser(inputToUse);
    auto result = parser.parseValue();
    parser.skipWhitespace();

    if (!parser.atEnd())
        parser.error("unexpected trailing content");

    return result;
}

} // namespace Miro::Json
