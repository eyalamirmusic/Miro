#include "Xml.h"

#include <iostream>

namespace Miro::Xml
{

void printTo(std::string& output, const Node& node, int indent, int depth);

void escapeInto(std::string& output, std::string_view text, bool isAttribute)
{
    for (auto c: text)
    {
        switch (c)
        {
            case '&':
                output += "&amp;";
                break;
            case '<':
                output += "&lt;";
                break;
            case '>':
                output += "&gt;";
                break;
            case '"':
                output += isAttribute ? "&quot;" : "\"";
                break;
            case '\'':
                output += isAttribute ? "&apos;" : "'";
                break;
            default:
                output += c;
        }
    }
}

void writeIndent(std::string& output, int indent, int depth)
{
    output += '\n';
    output.append(static_cast<std::size_t>(indent)
                      * static_cast<std::size_t>(depth),
                  ' ');
}

void printAttributes(std::string& output, const Node& node)
{
    for (const auto& [key, value]: node.attributes)
    {
        output += ' ';
        output += key;
        output += "=\"";
        escapeInto(output, value, true);
        output += '"';
    }
}

void printChildren(std::string& output, const Node& node, int indent, int depth)
{
    for (const auto& child: node.children)
    {
        if (indent > 0)
            writeIndent(output, indent, depth + 1);

        printTo(output, child, indent, depth + 1);
    }
}

void printTo(std::string& output, const Node& node, int indent, int depth)
{
    output += '<';
    output += node.name;
    printAttributes(output, node);

    auto hasChildren = !node.children.empty();
    auto hasText = !node.text.empty();

    if (!hasChildren && !hasText)
    {
        output += "/>";
        return;
    }

    output += '>';

    if (hasChildren)
    {
        printChildren(output, node, indent, depth);

        if (indent > 0)
            writeIndent(output, indent, depth);
    }
    else
    {
        escapeInto(output, node.text, false);
    }

    output += "</";
    output += node.name;
    output += '>';
}

std::string print(const Node& nodeToUse, int indentToUse)
{
    auto result = std::string {};
    printTo(result, nodeToUse, indentToUse, 0);
    return result;
}

void log(const Node& nodeToUse, int indentToUse)
{
    std::cout << print(nodeToUse, indentToUse) << std::endl;
}

} // namespace Miro::Xml
