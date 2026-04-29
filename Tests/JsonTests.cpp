#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro::Json;

auto parseNull = test("Parse null") = []
{
    auto value = parse("null");
    check(value.isNull());
};

auto parseBoolTrue = test("Parse bool true") = []
{
    auto value = parse("true");
    check(value.isBool());
    check(value.asBool() == true);
};

auto parseBoolFalse = test("Parse bool false") = []
{
    auto value = parse("false");
    check(value.isBool());
    check(value.asBool() == false);
};

auto parseInteger = test("Parse integer") = []
{
    auto value = parse("42");
    check(value.isNumber());
    check(value.asNumber() == 42.0);
};

auto parseNegativeNumber = test("Parse negative number") = []
{
    auto value = parse("-3.14");
    check(value.isNumber());
    check(value.asNumber() == -3.14);
};

auto parseExponent = test("Parse exponent") = []
{
    auto value = parse("1e10");
    check(value.isNumber());
    check(value.asNumber() == 1e10);
};

auto parseString = test("Parse string") = []
{
    auto value = parse(R"("hello")");
    check(value.isString());
    check(value.asString() == "hello");
};

auto parseStringEscapes = test("Parse string with escapes") = []
{
    auto value = parse(R"("line1\nline2\ttab")");
    check(value.asString() == "line1\nline2\ttab");
};

auto parseUnicodeEscape = test("Parse unicode escape") = []
{
    auto value = parse(R"("\u0041")");
    check(value.asString() == "A");
};

auto parseEmptyArray = test("Parse empty array") = []
{
    auto value = parse("[]");
    check(value.isArray());
    check(value.asArray().empty());
};

auto parseArray = test("Parse array") = []
{
    auto value = parse(R"([1, "two", true, null])");
    check(value.isArray());
    check(value.asArray().size() == 4);
    check(value[0].asNumber() == 1.0);
    check(value[1].asString() == "two");
    check(value[2].asBool() == true);
    check(value[3].isNull());
};

auto parseEmptyObject = test("Parse empty object") = []
{
    auto value = parse("{}");
    check(value.isObject());
    check(value.asObject().empty());
};

auto parseObject = test("Parse object") = []
{
    auto value = parse(R"({
        "name": "Miro",
        "version": 1.0,
        "features": ["json", "reflection"],
        "active": true,
        "metadata": null
    })");

    check(value.isObject());
    check(value["name"].asString() == "Miro");
    check(value["version"].asNumber() == 1.0);
    check(value["active"].asBool() == true);
    check(value["metadata"].isNull());
    check(value["features"].isArray());
    check(value["features"][0].asString() == "json");
    check(value["features"][1].asString() == "reflection");
};

auto parseNestedObjects = test("Parse nested objects") = []
{
    auto value = parse(R"({"a": {"b": {"c": 42}}})");
    check(value["a"]["b"]["c"].asNumber() == 42.0);
};

auto valueEquality = test("Value equality") = []
{
    check(parse("42") == parse("42"));
    check(parse(R"("hello")") == parse(R"("hello")"));
    check(parse("true") == parse("true"));
    check(parse("null") == parse("null"));
};

// --- Implicit conversion tests ---

auto implicitConvertToBool = test("Implicit convert to bool") = []
{
    Value value = true;
    bool result = value;
    check(result == true);

    Value falseValue = false;
    bool falseResult = falseValue;
    check(falseResult == false);
};

auto implicitConvertToInt = test("Implicit convert to int") = []
{
    Value value = 42;
    int result = value;
    check(result == 42);

    Value negative = -7;
    int negativeResult = negative;
    check(negativeResult == -7);
};

auto implicitConvertToDouble = test("Implicit convert to double") = []
{
    Value value = 3.14;
    double result = value;
    check(result == 3.14);
};

auto implicitConvertToFloat = test("Implicit convert to float") = []
{
    Value value = 1.5;
    float result = value;
    check(result == 1.5f);
};

auto implicitConvertToString = test("Implicit convert to string") = []
{
    Value value = std::string("hello");
    std::string result = value;
    check(result == "hello");
};

auto implicitConvertFromParsedObject =
    test("Implicit convert from parsed object") = []
{
    auto value = parse(R"({
        "active": true,
        "count": 42,
        "ratio": 2.5,
        "name": "Miro"
    })");

    bool active = value["active"];
    int count = value["count"];
    double ratio = value["ratio"];
    float ratioFloat = value["ratio"];
    std::string name = value["name"];

    check(active == true);
    check(count == 42);
    check(ratio == 2.5);
    check(ratioFloat == 2.5f);
    check(name == "Miro");
};

auto implicitConvertInExpression = test("Implicit convert in expression") = []
{
    Value value = 10;
    int doubled = static_cast<int>(value) * 2;
    check(doubled == 20);

    Value flag = true;
    if (flag)
        check(true);
    else
        check(false);
};

// --- Print tests ---

auto printNull = test("Print null") = [] { check(print(parse("null")) == "null"); };

auto printBool = test("Print bool") = []
{
    check(print(parse("true")) == "true");
    check(print(parse("false")) == "false");
};

auto printInteger = test("Print integer") = []
{
    check(print(parse("42")) == "42");
    check(print(parse("-7")) == "-7");
    check(print(parse("0")) == "0");
};

auto printFloat =
    test("Print float") = [] { check(print(parse("3.14")) == "3.14"); };

auto printString =
    test("Print string") = [] { check(print(parse(R"("hello")")) == R"("hello")"); };

auto printStringEscapes = test("Print string escapes") = []
{
    check(print(parse(R"("a\nb")")) == R"("a\nb")");
    check(print(parse(R"("a\tb")")) == R"("a\tb")");
    check(print(parse(R"("a\"b")")) == R"("a\"b")");
    check(print(parse(R"("a\\b")")) == R"("a\\b")");
};

auto printEmptyArray =
    test("Print empty array") = [] { check(print(parse("[]")) == "[]"); };

auto printArray =
    test("Print array") = [] { check(print(parse("[1,2,3]")) == "[1,2,3]"); };

auto printEmptyObject =
    test("Print empty object") = [] { check(print(parse("{}")) == "{}"); };

auto printObject = test("Print object") = []
{
    auto value = parse(R"({"a":1,"b":"two"})");
    check(print(value) == R"({"a":1,"b":"two"})");
};

auto printRoundtrip = test("Print roundtrip") = []
{
    auto input = R"({"active":true,"items":[1,"two",null],"name":"Miro"})";
    check(print(parse(input)) == input);
};

// --- Indented print tests ---

auto printIndentedObject = test("Print indented object") = []
{
    auto value = parse(R"({"a":1,"b":"two"})");
    auto expected = std::string {"{\n"
                                 "    \"a\": 1,\n"
                                 "    \"b\": \"two\"\n"
                                 "}"};
    check(print(value, 4) == expected);
};

auto printIndentedArray = test("Print indented array") = []
{
    auto value = parse("[1,2,3]");
    auto expected = std::string {"[\n"
                                 "    1,\n"
                                 "    2,\n"
                                 "    3\n"
                                 "]"};
    check(print(value, 4) == expected);
};

auto printIndentedNested = test("Print indented nested") = []
{
    auto value = parse(R"({"a":{"b":1},"c":[2,3]})");
    auto expected = std::string {"{\n"
                                 "    \"a\": {\n"
                                 "        \"b\": 1\n"
                                 "    },\n"
                                 "    \"c\": [\n"
                                 "        2,\n"
                                 "        3\n"
                                 "    ]\n"
                                 "}"};
    check(print(value, 4) == expected);
};

auto printIndentedEmpty = test("Print indented empty") = []
{
    check(print(parse("[]"), 4) == "[]");
    check(print(parse("{}"), 4) == "{}");
};

auto printIndentedScalar = test("Print indented scalar") = []
{
    check(print(parse("42"), 4) == "42");
    check(print(parse("\"hello\""), 4) == "\"hello\"");
    check(print(parse("true"), 4) == "true");
    check(print(parse("null"), 4) == "null");
};
