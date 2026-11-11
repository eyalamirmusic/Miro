// Pins the default behaviour of parse() and print() that the opt-in
// print and parse options sit beside, so a default-constructed option
// set can be checked against exactly what the plain calls do.

#include <Miro/Json.h>
#include <NanoTest/NanoTest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

using namespace nano;
using namespace Miro::Json;

namespace JsonDefaultsTests
{

using namespace std::string_view_literals;

bool parseThrows(std::string_view text)
{
    try
    {
        parse(text);
    }
    catch (const ParseError&)
    {
        return true;
    }

    return false;
}

std::string parseErrorMessage(std::string_view text)
{
    try
    {
        parse(text);
    }
    catch (const ParseError& error)
    {
        return error.what();
    }

    return {};
}

bool isNegativeZero(const Value& value)
{
    return !value.isInteger() && value.isNumber() && value.asNumber() == 0.0
           && std::signbit(value.asNumber());
}

// --- Layout ---

auto compactNested = test("Defaults: indent 0 prints fully compact") = []
{
    auto value = parse(R"({ "a" : [ 1 , { "b" : [ ] } ] , "c" : { } , "d" : [ ] })");
    auto expected = R"({"a":[1,{"b":[]}],"c":{},"d":[]})";

    check(print(value) == expected);
    check(print(value, 0) == expected);
};

auto negativeIndentIsCompact =
    test("Defaults: a negative indent prints compact") = []
{
    auto value = parse(R"({"a":[1,2],"b":{}})");
    check(print(value, -2) == R"({"a":[1,2],"b":{}})");
};

auto indentedNestedEmpties =
    test("Defaults: indent n puts empties inline at every depth") = []
{
    auto value = parse(R"({"a":[],"b":{},"c":[[],{}],"d":{"e":[1]}})");
    auto expected = std::string {"{\n"
                                 "  \"a\": [],\n"
                                 "  \"b\": {},\n"
                                 "  \"c\": [\n"
                                 "    [],\n"
                                 "    {}\n"
                                 "  ],\n"
                                 "  \"d\": {\n"
                                 "    \"e\": [\n"
                                 "      1\n"
                                 "    ]\n"
                                 "  }\n"
                                 "}"};

    check(print(value, 2) == expected);
};

auto indentOneLayout = test("Defaults: indent 1 layout") = []
{
    auto value = parse(R"([{"k":"v"},[]])");
    auto expected = std::string {"[\n"
                                 " {\n"
                                 "  \"k\": \"v\"\n"
                                 " },\n"
                                 " []\n"
                                 "]"};

    check(print(value, 1) == expected);
};

// --- Printing doubles ---

auto integralDoublePrintsAsInteger =
    test("Defaults: an integral double prints without a point") = []
{
    check(print(Value {1.0}) == "1");
    check(print(Value {-3.0}) == "-3");
    check(print(Value {100.0}) == "100");
    check(print(parse("1.0")) == "1");
    check(print(parse("1e2")) == "100");
};

auto negativeZeroPrintsAsZero = test("Defaults: -0.0 prints as 0") = []
{
    check(print(Value {-0.0}) == "0");
    check(print(parse("-0.0")) == "0");
};

auto largeIntegralDoubles = test("Defaults: large integral doubles") = []
{
    check(print(Value {1e15}) == "1000000000000000");
    check(print(Value {1e20}) == "1e+20");
    check(print(Value {1e300}) == "1e+300");
    check(print(Value {-1e20}) == "-1e+20");
};

auto smallestSubnormal = test("Defaults: the smallest subnormal") = []
{
    auto smallest = std::numeric_limits<double>::denorm_min();
    check(print(Value {smallest}) == "4.94065645841247e-324");
    check(parse("4.94065645841247e-324").asNumber() == smallest);
};

auto nonFiniteInContainers =
    test("Defaults: NaN and infinities print as null inside containers") = []
{
    auto array = Array {};
    array.add(std::numeric_limits<double>::quiet_NaN());
    array.add(std::numeric_limits<double>::infinity());
    array.add(-std::numeric_limits<double>::infinity());

    check(print(Value {array}) == "[null,null,null]");
};

auto int64ExtremesConstructed =
    test("Defaults: int64 extremes print exactly when constructed") = []
{
    auto lowest = std::numeric_limits<std::int64_t>::min();
    auto largest = std::numeric_limits<std::int64_t>::max();

    check(print(Value {lowest}) == "-9223372036854775808");
    check(print(Value {largest}) == "9223372036854775807");
};

// --- What the printed tokens parse back to ---

auto tokensParseBack = test("Defaults: which alternative each token parses to") = []
{
    check(parse("1").isInteger());
    check(parse("1").asInteger() == 1);

    check(parse("-0").isInteger());
    check(parse("-0").asInteger() == 0);

    check(isNegativeZero(parse("-0.0")));

    check(!parse("1.0").isInteger());
    check(parse("1.0").asNumber() == 1.0);

    check(!parse("1e2").isInteger());
    check(parse("1e2").asNumber() == 100.0);

    check(parse("1000000000000000").isInteger());
    check(parse("1000000000000000").asInteger() == 1000000000000000LL);

    check(!parse("1e+20").isInteger());
    check(parse("1e+20").asNumber() == 1e20);

    check(!parse("1e+300").isInteger());
    check(parse("1e+300").asNumber() == 1e300);

    check(!parse("4.94065645841247e-324").isInteger());

    check(parse("null").isNull());

    check(parse("-9223372036854775808").isInteger());
    check(parse("9223372036854775807").isInteger());
};

// --- Comments ---

auto commentsEverywhere =
    test("Defaults: comments are accepted wherever whitespace is") = []
{
    auto text = "/*a*/ // b\n"
                "{ /*c*/ \"k\" /*d*/ : /*e*/ [ /*f*/ 1 /*g*/ , // h\n"
                "2 // i\r"
                "] /*j*/ , \"e\" : { /**/ } , \"f\" : [ // k\n"
                "] } // tail";

    auto value = parse(text);

    check(value["k"].asArray().size() == 2);
    check(value["k"][1].asInteger() == 2);
    check(value["e"].asObject().empty());
    check(value["f"].asArray().empty());
};

auto lineCommentEndings =
    test("Defaults: a line comment ends at LF, CR or end of input") = []
{
    check(parse("// x\n1").asInteger() == 1);
    check(parse("// x\r1").asInteger() == 1);
    check(parse("1 // x").asInteger() == 1);
    check(parse("1 //").asInteger() == 1);
};

auto blockCommentForms = test("Defaults: block comment forms") = []
{
    check(parse("/**/1").asInteger() == 1);
    check(parse("/***/1").asInteger() == 1);
    check(parse("/* a /* b */ 1").asInteger() == 1);
    check(parse("1/* x */").asInteger() == 1);
    check(parseThrows("/* a /* b */ */ 1"));
    check(parseThrows("1 /* open"));
    check(parseThrows("1 /"));
};

auto commentInsideStringIsText =
    test("Defaults: comment markers inside a string are text") = []
{
    check(parse(R"("// not a comment")").asString() == "// not a comment");
    check(parse(R"("/* nor this */")").asString() == "/* nor this */");
};

auto commentErrorMessages = test("Defaults: comment error messages") = []
{
    check(parseErrorMessage("/* never closes")
          == "JSON parse error at position 0: unterminated block comment");
    check(parseErrorMessage("1 /x")
          == "JSON parse error at position 2: unexpected character '/'");
};

// --- Duplicate keys ---

auto duplicateFirstWins = test("Defaults: the first duplicate key wins") = []
{
    auto value = parse(R"({"a":1,"a":"two","a":[3]})");

    check(value.asObject().size() == 1);
    check(value["a"].asInteger() == 1);

    auto nested = parse(R"({"o":{"x":true,"x":false},"o":null})");
    check(nested["o"]["x"].asBool() == true);

    check(getParsedValue(R"({"a":1,"a":2})")["a"].asInteger() == 1);
};

// --- NUL bytes ---

auto trailingNulIsAnError = test("Defaults: a NUL after the document throws") = []
{
    check(parseThrows("1\0"sv));
    check(parseThrows("{}\0"sv));
    check(parseThrows("[1] \0"sv));
    check(parseThrows("\"s\"\0\0"sv));
    check(parseErrorMessage("1\0"sv)
          == "JSON parse error at position 1: unexpected trailing content");
};

auto nulOutsideStringIsAnError =
    test("Defaults: a NUL anywhere outside a string throws") = []
{
    check(parseThrows("\0"sv));
    check(parseThrows("\0 1"sv));
    check(parseThrows("[\0 1]"sv));
    check(parseThrows("[1\0]"sv));
    check(parseThrows("{\"a\"\0:1}"sv));
    check(parseThrows("{\"a\":\0 1}"sv));
    check(parseThrows("tr\0ue"sv));
    check(parseThrows("1\0 2"sv));
};

auto nulInsideStringIsKept = test("Defaults: a NUL inside a string is kept") = []
{
    auto value = parse("\"a\0b\""sv);
    check(value.asString() == "a\0b"sv);
};

auto nulThroughGetParsedValue =
    test("Defaults: getParsedValue returns null for a NUL outside a string") = []
{
    check(getParsedValue("1\0"sv).isNull());
    check(getParsedValue("[\0]"sv).isNull());
    check(getParsedValue("{\"a\":1}\0"sv).isNull());
    check(getParsedValue("\"a\0b\""sv).asString() == "a\0b"sv);
};

} // namespace JsonDefaultsTests
