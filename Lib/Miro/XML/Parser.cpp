#include "Xml.h"

namespace Miro::Xml
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

    Node parseDocument()
    {
        skipWhitespace();

        if (atEnd())
            error("expected root element");

        return parseElement();
    }

    void skipWhitespace()
    {
        while (pos < end
               && (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r'))
        {
            ++pos;
        }
    }

    bool atEnd() const { return pos >= end; }

    [[noreturn]] void error(const std::string& messageToUse) const
    {
        throw ParseError("XML parse error at position "
                         + std::to_string(pos - input) + ": " + messageToUse);
    }

private:
    Node parseElement()
    {
        expect('<');

        auto node = Node {};
        node.name = parseName();

        parseAttributes(node);

        if (consume("/>"))
            return node;

        expect('>');

        parseBody(node);

        return node;
    }

    void parseAttributes(Node& node)
    {
        while (pos < end)
        {
            skipWhitespace();

            if (pos < end && (*pos == '>' || *pos == '/'))
                return;

            auto key = parseName();
            skipWhitespace();
            expect('=');
            skipWhitespace();
            auto value = parseAttributeValue();
            node.attributes.emplace(std::move(key), std::move(value));
        }
    }

    std::string parseAttributeValue()
    {
        if (pos >= end || (*pos != '"' && *pos != '\''))
            error("expected quoted attribute value");

        auto quote = *pos;
        ++pos;

        auto value = std::string {};

        while (pos < end && *pos != quote)
        {
            if (*pos == '&')
                value += parseEntity();
            else if (*pos == '<')
                error("'<' is not allowed in attribute value");
            else
                value += *pos++;
        }

        if (pos >= end)
            error("unterminated attribute value");

        ++pos;
        return value;
    }

    void parseBody(Node& node)
    {
        auto text = std::string {};

        while (pos < end)
        {
            if (startsWith("</"))
            {
                if (!node.children.empty())
                    text.clear();

                node.text = std::move(text);
                parseClosingTag(node.name);
                return;
            }

            if (*pos == '<')
            {
                node.children.add(parseElement());
            }
            else if (*pos == '&')
            {
                text += parseEntity();
            }
            else
            {
                text += *pos++;
            }
        }

        error("unterminated element");
    }

    void parseClosingTag(std::string_view expectedName)
    {
        expect('<');
        expect('/');
        auto name = parseName();

        if (name != expectedName)
            error("mismatched closing tag </" + name + ">, expected </"
                  + std::string(expectedName) + ">");

        skipWhitespace();
        expect('>');
    }

    std::string parseName()
    {
        if (pos >= end || !isNameStart(*pos))
            error("expected name");

        auto start = pos;
        ++pos;

        while (pos < end && isNameChar(*pos))
            ++pos;

        return {start, static_cast<std::size_t>(pos - start)};
    }

    std::string parseEntity()
    {
        expect('&');

        auto start = pos;

        while (pos < end && *pos != ';')
            ++pos;

        if (pos >= end)
            error("unterminated entity reference");

        auto name = std::string_view(start, static_cast<std::size_t>(pos - start));
        ++pos;

        if (name == "amp")
            return "&";
        if (name == "lt")
            return "<";
        if (name == "gt")
            return ">";
        if (name == "quot")
            return "\"";
        if (name == "apos")
            return "'";

        error("unknown entity '&" + std::string(name) + ";'");
    }

    static bool isNameStart(char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'
               || c == ':';
    }

    static bool isNameChar(char c)
    {
        return isNameStart(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
    }

    bool startsWith(const char* literal) const
    {
        auto len = std::size_t {0};

        while (literal[len] != '\0')
            ++len;

        if (static_cast<std::size_t>(end - pos) < len)
            return false;

        for (auto i = std::size_t {0}; i < len; ++i)
            if (pos[i] != literal[i])
                return false;

        return true;
    }

    bool consume(const char* literal)
    {
        if (!startsWith(literal))
            return false;

        while (*literal != '\0')
        {
            ++pos;
            ++literal;
        }

        return true;
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

Node parse(std::string_view inputToUse)
{
    auto parser = Parser(inputToUse);
    auto result = parser.parseDocument();
    parser.skipWhitespace();

    if (!parser.atEnd())
        parser.error("unexpected trailing content");

    return result;
}

} // namespace Miro::Xml
