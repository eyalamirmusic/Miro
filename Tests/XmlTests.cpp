#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;
using namespace Miro::Xml;

auto printLeafSelfClosing = test("Print empty leaf as self-closing") = []
{
    auto node = Node {.name = "x"};
    check(print(node) == "<x/>");
};

auto printLeafWithText = test("Print leaf with text") = []
{
    auto node = Node {.name = "x", .text = "hello"};
    check(print(node) == "<x>hello</x>");
};

auto printAttributes = test("Print attributes") = []
{
    auto node = Node {.name = "Person"};
    node.attributes["name"] = "Alice";
    node.attributes["age"] = "30";
    check(print(node) == R"(<Person age="30" name="Alice"/>)");
};

auto printNestedFlat = test("Print nested children flat") = []
{
    auto child = Node {.name = "name", .text = "Alice"};
    auto root = Node {.name = "Person"};
    root.children.add(child);
    check(print(root) == "<Person><name>Alice</name></Person>");
};

auto printNestedIndented = test("Print nested children indented") = []
{
    auto child = Node {.name = "name", .text = "Alice"};
    auto root = Node {.name = "Person"};
    root.children.add(child);

    auto expected = std::string {
        "<Person>\n"
        "  <name>Alice</name>\n"
        "</Person>"};

    check(print(root, 2) == expected);
};

auto printEscapesText = test("Print escapes text content") = []
{
    auto node = Node {.name = "x", .text = "5 < 6 & ok"};
    check(print(node) == "<x>5 &lt; 6 &amp; ok</x>");
};

auto printEscapesAttributes = test("Print escapes attribute values") = []
{
    auto node = Node {.name = "x"};
    node.attributes["q"] = R"(He said "hi" & left)";
    check(print(node) == R"(<x q="He said &quot;hi&quot; &amp; left"/>)");
};

auto parseSelfClosing = test("Parse self-closing tag") = []
{
    auto node = parse("<x/>");
    check(node.name == "x");
    check(node.children.empty());
    check(node.text.empty());
    check(node.attributes.empty());
};

auto parseOpenClose = test("Parse open/close tag") = []
{
    auto node = parse("<x></x>");
    check(node.name == "x");
    check(node.text.empty());
};

auto parseLeafWithText = test("Parse leaf with text") = []
{
    auto node = parse("<x>hello</x>");
    check(node.text == "hello");
};

auto parseAttributes = test("Parse attributes (both quote styles)") = []
{
    auto node = parse(R"(<Person name="Alice" age='30'/>)");
    check(*findAttribute(node, "name") == "Alice");
    check(*findAttribute(node, "age") == "30");
};

auto parseEntities = test("Parse entity escapes") = []
{
    auto node = parse("<x>5 &lt; 6 &amp; &quot;ok&quot;</x>");
    check(node.text == R"(5 < 6 & "ok")");
};

auto parseEntitiesInAttribute = test("Parse entity escapes in attribute") = []
{
    auto node = parse(R"(<x q="&amp;&lt;"/>)");
    check(*findAttribute(node, "q") == "&<");
};

auto parseNested = test("Parse nested children") = []
{
    auto node = parse(R"(<Person><name>Alice</name><age>30</age></Person>)");
    check(node.children.size() == 2);
    check(node.children[0].name == "name");
    check(node.children[0].text == "Alice");
    check(node.children[1].name == "age");
    check(node.children[1].text == "30");
};

auto parseNestedWithWhitespace = test("Parse nested children with whitespace") = []
{
    auto input = std::string {
        "<Person>\n"
        "  <name>Alice</name>\n"
        "</Person>"};
    auto node = parse(input);
    check(node.children.size() == 1);
    check(node.children[0].name == "name");
    check(node.children[0].text == "Alice");
    check(node.text.empty());
};

auto roundTripIndented = test("Round-trip indented document") = []
{
    auto root = Node {.name = "Person"};
    root.attributes["id"] = "42";
    root.children.add(Node {.name = "name", .text = "Alice"});
    root.children.add(Node {.name = "note", .text = "5 < 6 & ok"});

    auto serialized = print(root, 2);
    auto reparsed = parse(serialized);
    check(reparsed == root);
};

auto parseRejectsMismatch = test("Parse rejects mismatched closing tag") = []
{
    auto threw = false;

    try
    {
        parse("<x></y>");
    }
    catch (const ParseError&)
    {
        threw = true;
    }

    check(threw);
};

auto parseRejectsTrailing = test("Parse rejects trailing content") = []
{
    auto threw = false;

    try
    {
        parse("<x/>garbage");
    }
    catch (const ParseError&)
    {
        threw = true;
    }

    check(threw);
};

auto parseRejectsUnterminated = test("Parse rejects unterminated tag") = []
{
    auto threw = false;

    try
    {
        parse("<x>");
    }
    catch (const ParseError&)
    {
        threw = true;
    }

    check(threw);
};
