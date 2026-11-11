#include <Miro/Json.h>
#include <NanoTest/NanoTest.h>

#include <array>
#include <clocale>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <vector>

using namespace nano;
using namespace Miro::Json;

namespace JsonOptionsTests
{

using namespace std::string_view_literals;

PrintOptions doublesKept()
{
    auto options = PrintOptions {};
    options.keepDoublesAsDoubles = true;
    return options;
}

PrintOptions lineBroken()
{
    auto options = PrintOptions {};
    options.breakLines = true;
    return options;
}

PrintOptions indented(int indentToUse)
{
    auto options = PrintOptions {};
    options.indent = indentToUse;
    return options;
}

ParseOptions commentsRejected()
{
    auto options = ParseOptions {};
    options.allowComments = false;
    return options;
}

ParseOptions lastKeyWins()
{
    auto options = ParseOptions {};
    options.duplicateKeys = ParseOptions::DuplicateKeys::KeepLast;
    return options;
}

ParseOptions nulTerminated()
{
    auto options = ParseOptions {};
    options.endAtNul = true;
    return options;
}

std::string parseErrorMessage(std::string_view text, const ParseOptions& options)
{
    try
    {
        parseWithOptions(text, options);
    }
    catch (const ParseError& error)
    {
        return error.what();
    }

    return {};
}

std::string commentsError(std::size_t position)
{
    return "JSON parse error at position " + std::to_string(position)
           + ": comments are not allowed";
}

std::uint64_t bitsOf(double number)
{
    auto bits = std::uint64_t {};
    std::memcpy(&bits, &number, sizeof bits);
    return bits;
}

double doubleFromBits(std::uint64_t bits)
{
    auto number = 0.0;
    std::memcpy(&number, &bits, sizeof number);
    return number;
}

std::string withoutLeadingSpaces(const std::string& text)
{
    auto result = std::string {};
    auto atLineStart = true;

    for (auto c: text)
    {
        if (atLineStart && c == ' ')
            continue;

        atLineStart = c == '\n';
        result += c;
    }

    return result;
}

// A spelling that tells the integer and double alternatives apart and
// keeps the sign of zero, so two values that print alike are the same.
std::string exactly(const Value& value)
{
    return printWithOptions(value, doublesKept());
}

// The value a text parses to, or the exact error it throws.
std::string outcomeOf(std::string_view text, const ParseOptions& options)
{
    try
    {
        return "value: " + exactly(parseWithOptions(text, options));
    }
    catch (const ParseError& error)
    {
        return std::string("error: ") + error.what();
    }
}

Value sampleDocument()
{
    return parse(R"({
        "empty": {},
        "list": [],
        "nested": {"a": [1, 2.5, [], {}], "b": {"c": null}},
        "number": 1.0,
        "scalars": [true, false, null, "text", -7, 0.1],
        "string": "with \"quotes\" and \n newline",
        "zero": -0.0
    })");
}

std::vector<Value> sampleValues()
{
    auto values = std::vector<Value> {};
    values.push_back(sampleDocument());
    values.push_back(parse("[]"));
    values.push_back(parse("{}"));
    values.push_back(parse("[[[]]]"));
    values.push_back(parse(R"({"a":{"b":{"c":{}}}})"));
    values.push_back(parse(R"([1,[2,[3,{"k":[4]}]]])"));
    values.push_back(
        parse(R"({"  lead": "  spaced", "k": ["  x", " \t\\ \"q\""]})"));
    values.push_back(Value {42});
    values.push_back(Value {1.5});
    values.push_back(Value {"scalar"});
    values.push_back(Value {});
    return values;
}

// The parse options the defaults must be checked against, as documents
// that each exercise one of them.
std::vector<std::string_view> sampleTexts()
{
    return {
        "null",
        R"({"a":1,"a":2,"b":[1,2.0,-0,-0.0,1e2]})",
        "// lead\n{ /* in */ \"k\" : [1 , 2] } // tail",
        "\"a\0b\""sv,
        "1\0"sv,
        "[\0]"sv,
        "/* open",
        "/ 1",
        "{\"a\" 1}",
        "",
        "[1,]",
    };
}

// --- Generated documents ---

using Engine = std::mt19937_64;

int pick(Engine& engine, int lowest, int highest)
{
    return std::uniform_int_distribution<int> {lowest, highest}(engine);
}

std::string randomString(Engine& engine)
{
    constexpr auto pieces = std::array<std::string_view, 16> {" ",
                                                              "  lead",
                                                              "a",
                                                              "key",
                                                              "\\",
                                                              "\"",
                                                              "\n",
                                                              "\t",
                                                              "\x01",
                                                              "\xc3\xa9",
                                                              "//",
                                                              "/*",
                                                              "*/",
                                                              "x y",
                                                              "\0"sv,
                                                              "\\u0061"};

    auto text = std::string {};

    for (auto count = pick(engine, 0, 4); count > 0; --count)
        text += pieces[static_cast<std::size_t>(pick(engine, 0, 15))];

    return text;
}

double randomDouble(Engine& engine)
{
    switch (pick(engine, 0, 5))
    {
        case 0:
            return static_cast<double>(pick(engine, -1000, 1000));
        case 1:
            return -0.0;
        case 2:
            return std::ldexp(static_cast<double>(pick(engine, 1, 1 << 20)),
                              pick(engine, 0, 300));
        case 3:
            return static_cast<double>(pick(engine, -1000, 1000)) / 8.0;
        case 4:
            return std::numeric_limits<double>::denorm_min()
                   * static_cast<double>(pick(engine, 1, 1000));
        default:
        {
            auto number = doubleFromBits(engine());
            return std::isfinite(number) ? number : 0.5;
        }
    }
}

Value randomValue(Engine& engine, int depth)
{
    switch (pick(engine, 0, depth > 0 ? 7 : 4))
    {
        case 0:
            return Value {};
        case 1:
            return Value {pick(engine, 0, 1) == 1};
        case 2:
            return Value {static_cast<std::int64_t>(engine())
                          >> pick(engine, 0, 63)};
        case 3:
            return Value {randomDouble(engine)};
        case 4:
            return Value {randomString(engine)};
        case 5:
        case 6:
        {
            auto array = Array {};

            for (auto count = pick(engine, 0, 4); count > 0; --count)
                array.add(randomValue(engine, depth - 1));

            return Value {std::move(array)};
        }
        default:
        {
            auto object = Object {};

            for (auto count = pick(engine, 0, 4); count > 0; --count)
                object[randomString(engine)] = randomValue(engine, depth - 1);

            return Value {std::move(object)};
        }
    }
}

PrintOptions randomLayout(Engine& engine)
{
    auto options = PrintOptions {};
    options.indent = pick(engine, -1, 4);
    options.breakLines = pick(engine, 0, 1) == 1;
    options.keepDoublesAsDoubles = pick(engine, 0, 1) == 1;
    return options;
}

// `text` with a few short insertions at random places, any of which can
// land inside a string: whitespace, comments, NULs and stray syntax.
std::string mutated(Engine& engine, std::string text)
{
    constexpr auto insertions = std::array<std::string_view, 12> {" ",
                                                                  "\n",
                                                                  "// c\n",
                                                                  "/* c */",
                                                                  "/**/",
                                                                  "/",
                                                                  "\0"sv,
                                                                  "\0 x"sv,
                                                                  "x",
                                                                  ",",
                                                                  "]",
                                                                  "//"};

    for (auto count = pick(engine, 1, 3); count > 0; --count)
    {
        auto at =
            static_cast<std::size_t>(pick(engine, 0, static_cast<int>(text.size())));
        auto insertion = insertions[static_cast<std::size_t>(pick(engine, 0, 11))];
        text.insert(at, insertion);
    }

    return text;
}

std::vector<Value> generatedValues(std::uint64_t seed, int count)
{
    auto engine = Engine {seed};
    auto values = std::vector<Value> {};

    for (auto i = 0; i < count; ++i)
        values.push_back(randomValue(engine, pick(engine, 0, 5)));

    return values;
}

Value deeplyNested(int depth)
{
    auto leaf = Array {};
    leaf.add(1.0);
    leaf.add(-0.0);
    leaf.add("  starts with spaces");
    leaf.add(" \t\\ \"quoted\" \n ");

    auto value = Value {std::move(leaf)};

    for (auto level = 0; level < depth; ++level)
    {
        if (level % 2 == 0)
        {
            auto array = Array {};
            array.add(std::move(value));
            array.add(2.0);
            value = Value {std::move(array)};
        }
        else
        {
            auto object = Object {};
            object["  key"] = std::move(value);
            object["v"] = "   ";
            value = Value {std::move(object)};
        }
    }

    return value;
}

// --- Defaults ---

auto defaultPrintOptions =
    test("PrintOptions: defaults print exactly as print(value, n)") = []
{
    for (const auto& value: sampleValues())
    {
        check(printWithOptions(value, PrintOptions {}) == print(value));

        for (auto indent: {-3, -1, 0, 1, 2, 4, 8})
            check(printWithOptions(value, indented(indent)) == print(value, indent));
    }

    for (auto number: {1.0, -0.0, 0.0, 1e15, 1e20, 1e300, 0.1, 5e-324})
        check(printWithOptions(Value {number}, PrintOptions {})
              == print(Value {number}));
};

auto defaultParseOptions =
    test("ParseOptions: defaults parse exactly as parse(text)") = []
{
    for (auto text: sampleTexts())
    {
        auto plain = getParsedValue(text);
        auto withOptions = getParsedValueWithOptions(text, ParseOptions {});

        check(print(withOptions) == print(plain));

        auto plainError = std::string {};

        try
        {
            parse(text);
        }
        catch (const ParseError& error)
        {
            plainError = error.what();
        }

        check(parseErrorMessage(text, ParseOptions {}) == plainError);
    }
};

// --- Doubles stay doubles ---

auto integralDoublesKeepAFraction =
    test("PrintOptions: integral doubles keep a fractional part") = []
{
    auto options = doublesKept();

    check(printWithOptions(Value {1.0}, options) == "1.0");
    check(printWithOptions(Value {0.0}, options) == "0.0");
    check(printWithOptions(Value {-0.0}, options) == "-0.0");
    check(printWithOptions(Value {-3.0}, options) == "-3.0");
    check(printWithOptions(Value {123456789.0}, options) == "123456789.0");
    check(printWithOptions(Value {1e15}, options) == "1e+15");
    check(printWithOptions(Value {1e20}, options) == "1e+20");
    check(printWithOptions(Value {-1e20}, options) == "-1e+20");
    check(printWithOptions(Value {1e300}, options) == "1e+300");
    check(printWithOptions(Value {9007199254740992.0}, options)
          == "9007199254740992.0");
    check(printWithOptions(Value {std::numeric_limits<double>::max()}, options)
          == "1.7976931348623157e+308");
};

auto keptDoublesParseBackAsDoubles =
    test("PrintOptions: kept doubles parse back as the same double") = []
{
    auto options = doublesKept();

    for (auto number: {1.0, 0.0, -0.0, -3.0, 1e15, 1e20, 1e300, 4503599627370496.0})
    {
        auto back = parse(printWithOptions(Value {number}, options));

        check(!back.isInteger());
        check(bitsOf(back.asNumber()) == bitsOf(number));
    }
};

auto keptDoublesLeaveTheRestAlone =
    test("PrintOptions: integers, non-integral and non-finite unaffected") = []
{
    auto options = doublesKept();
    auto nan = std::numeric_limits<double>::quiet_NaN();
    auto infinity = std::numeric_limits<double>::infinity();

    check(printWithOptions(Value {1}, options) == "1");
    check(printWithOptions(Value {0}, options) == "0");
    check(printWithOptions(parse("-0"), options) == "0");
    check(printWithOptions(Value {std::numeric_limits<std::int64_t>::min()}, options)
          == "-9223372036854775808");
    check(printWithOptions(Value {std::numeric_limits<std::int64_t>::max()}, options)
          == "9223372036854775807");

    check(printWithOptions(Value {nan}, options) == "null");
    check(printWithOptions(Value {infinity}, options) == "null");
    check(printWithOptions(Value {-infinity}, options) == "null");

    for (auto number: {0.1, 1.0 / 3.0, 123456.789, 0.1 + 0.2, 5e-324, 1e-7, -2.5})
        check(printWithOptions(Value {number}, options) == print(Value {number}));
};

auto keptDoublesInContainers =
    test("PrintOptions: kept doubles inside containers") = []
{
    auto array = Array {};
    array.add(1.0);
    array.add(1);
    array.add(-0.0);
    array.add(Array {});

    auto object = Object {};
    object["d"] = 2.0;
    object["i"] = 2;
    object["list"] = Value {array};

    auto printed = printWithOptions(Value {object}, doublesKept());
    check(printed == R"({"d":2.0,"i":2,"list":[1.0,1,-0.0,[]]})");

    auto back = parse(printed);
    check(!back["d"].isInteger());
    check(back["i"].isInteger());
    check(!back["list"][0].isInteger());
    check(back["list"][1].isInteger());
    check(std::signbit(back["list"][2].asNumber()));
};

auto randomDoublesRoundTrip =
    test("PrintOptions: random doubles round-trip bit-exact as doubles") = []
{
    auto engine = std::mt19937_64 {20261111};
    auto options = doublesKept();
    auto failures = 0;

    auto checkOne = [&](double number)
    {
        if (!std::isfinite(number))
            return;

        auto token = printWithOptions(Value {number}, options);
        auto back = parse(token);

        auto ok = !back.isInteger() && bitsOf(back.asNumber()) == bitsOf(number)
                  && token.find_first_of(".eE") != std::string::npos;

        if (number != std::trunc(number))
            ok = ok && token == print(Value {number});

        if (!ok && failures++ < 10)
            std::cout << "  mismatch: " << token << "\n";
    };

    auto integral = std::uniform_int_distribution<std::int64_t> {
        std::numeric_limits<std::int64_t>::min(),
        std::numeric_limits<std::int64_t>::max()};
    auto small = std::uniform_int_distribution<int> {-100000, 100000};
    auto exponent = std::uniform_int_distribution<int> {-330, 308};
    auto unit = std::uniform_real_distribution<double> {-10.0, 10.0};
    auto mantissa = std::uniform_int_distribution<std::uint64_t> {
        1, (std::uint64_t {1} << 52) - 1};
    auto anyBits = std::uniform_int_distribution<std::uint64_t> {};

    for (auto i = 0; i < 20000; ++i)
    {
        auto whole = static_cast<double>(integral(engine) >> (i % 64));
        checkOne(whole);
        checkOne(-whole);
        checkOne(static_cast<double>(small(engine)));

        checkOne(std::nextafter(whole, 0.0));
        checkOne(std::nextafter(whole, std::numeric_limits<double>::infinity()));
        checkOne(std::nextafter(static_cast<double>(small(engine)), 1e9));

        auto huge = std::ldexp(unit(engine), 1000 + i % 23);
        checkOne(huge);
        checkOne(std::trunc(huge));
        checkOne(std::pow(10.0, exponent(engine)));

        checkOne(doubleFromBits(mantissa(engine)));
        checkOne(-doubleFromBits(mantissa(engine)));
        checkOne(doubleFromBits(anyBits(engine)));
    }

    checkOne(-0.0);
    checkOne(0.0);
    checkOne(std::numeric_limits<double>::denorm_min());
    checkOne(std::numeric_limits<double>::min());
    checkOne(std::numeric_limits<double>::max());
    checkOne(-std::numeric_limits<double>::max());
    checkOne(9223372036854775808.0);
    checkOne(-9223372036854775808.0);

    check(failures == 0);
};

// --- Line-broken layout ---

auto lineBrokenLayout = test("PrintOptions: line-broken layout") = []
{
    auto value = parse(R"({"a":[1,{"b":null}],"c":{},"d":[],"e":"x"})");
    auto expected = std::string {"{\n"
                                 "\"a\": [\n"
                                 "1,\n"
                                 "{\n"
                                 "\"b\": null\n"
                                 "}\n"
                                 "],\n"
                                 "\"c\": {},\n"
                                 "\"d\": [],\n"
                                 "\"e\": \"x\"\n"
                                 "}"};

    check(printWithOptions(value, lineBroken()) == expected);
};

auto lineBrokenEmptiesAndScalars =
    test("PrintOptions: line-broken empties and scalars") = []
{
    auto options = lineBroken();

    check(printWithOptions(parse("[]"), options) == "[]");
    check(printWithOptions(parse("{}"), options) == "{}");
    check(printWithOptions(parse("[[]]"), options) == "[\n[]\n]");
    check(printWithOptions(parse(R"({"a":{}})"), options) == "{\n\"a\": {}\n}");
    check(printWithOptions(Value {42}, options) == "42");
    check(printWithOptions(Value {"s"}, options) == "\"s\"");
    check(printWithOptions(Value {}, options) == "null");
};

auto lineBrokenIsIndentedWithoutSpaces =
    test("PrintOptions: line-broken equals indent n without leading spaces") = []
{
    for (const auto& value: sampleValues())
    {
        auto broken = printWithOptions(value, lineBroken());

        for (auto indent: {1, 2, 3, 4, 8})
            check(broken == withoutLeadingSpaces(print(value, indent)));
    }
};

auto lineBrokenWithIndent =
    test("PrintOptions: breakLines beside an indent changes nothing") = []
{
    for (const auto& value: sampleValues())
    {
        for (auto indent: {1, 2, 4})
        {
            auto options = indented(indent);
            options.breakLines = true;
            check(printWithOptions(value, options) == print(value, indent));
        }

        for (auto indent: {0, -1, -4})
        {
            auto options = indented(indent);
            options.breakLines = true;
            check(printWithOptions(value, options)
                  == printWithOptions(value, lineBroken()));
        }
    }
};

auto lineBrokenWithKeptDoubles =
    test("PrintOptions: line-broken layout with kept doubles") = []
{
    auto options = lineBroken();
    options.keepDoublesAsDoubles = true;

    auto value = parse(R"({"a":[1,1.0,-0.0],"b":2.5})");
    auto expected = std::string {"{\n"
                                 "\"a\": [\n"
                                 "1,\n"
                                 "1.0,\n"
                                 "-0.0\n"
                                 "],\n"
                                 "\"b\": 2.5\n"
                                 "}"};

    check(printWithOptions(value, options) == expected);

    auto indentedKept = indented(4);
    indentedKept.keepDoublesAsDoubles = true;
    check(printWithOptions(value, options)
          == withoutLeadingSpaces(printWithOptions(value, indentedKept)));
};

auto lineBrokenStringsWithSpaces =
    test("PrintOptions: line-broken strings keep their own spaces") = []
{
    auto value = parse(R"({"  k": ["  lead", " \t\\ \"q\" ", "   "]})");
    auto expected = std::string {"{\n"
                                 "\"  k\": [\n"
                                 "\"  lead\",\n"
                                 "\" \\t\\\\ \\\"q\\\" \",\n"
                                 "\"   \"\n"
                                 "]\n"
                                 "}"};

    check(printWithOptions(value, lineBroken()) == expected);
    check(parse(printWithOptions(value, lineBroken())) == value);
};

auto deeplyNestedLayouts =
    test("PrintOptions: deeply nested values under both print options") = []
{
    auto value = deeplyNested(64);
    auto options = lineBroken();
    options.keepDoublesAsDoubles = true;

    auto printed = printWithOptions(value, options);

    for (auto indent: {1, 2, 4})
    {
        auto indentedKept = indented(indent);
        indentedKept.keepDoublesAsDoubles = true;
        check(printed
              == withoutLeadingSpaces(printWithOptions(value, indentedKept)));
        check(printWithOptions(value, lineBroken())
              == withoutLeadingSpaces(print(value, indent)));
    }

    auto back = parse(printed);
    check(exactly(back) == exactly(value));
    check(printWithOptions(back, options) == printed);
};

auto printedDocumentRoundTrips =
    test("PrintOptions: every combination round-trips through parse") = []
{
    auto document = sampleDocument();

    for (auto breakLines: {false, true})
    {
        for (auto keepDoubles: {false, true})
        {
            for (auto indent: {0, 2})
            {
                auto options = indented(indent);
                options.breakLines = breakLines;
                options.keepDoublesAsDoubles = keepDoubles;

                auto back = parse(printWithOptions(document, options));
                check(back == document);
                check(printWithOptions(back, options)
                      == printWithOptions(document, options));
                check(back["number"].isInteger() == !keepDoubles);
                check(std::signbit(back["zero"].asNumber()) == keepDoubles);
            }
        }
    }
};

auto generatedPrintProperties =
    test("PrintOptions: properties over generated documents") = []
{
    for (const auto& value: generatedValues(7001, 400))
    {
        auto broken = printWithOptions(value, lineBroken());

        for (auto indent: {1, 3})
            check(broken == withoutLeadingSpaces(print(value, indent)));

        auto options = lineBroken();
        options.keepDoublesAsDoubles = true;

        auto keptBack = parse(printWithOptions(value, options));
        check(exactly(keptBack) == exactly(value));

        auto plainBack = parse(broken);
        check(plainBack == value);
        check(print(plainBack) == print(value));
    }
};

// --- Comments rejected ---

auto commentsRejectedEverywhere =
    test("ParseOptions: rejected comments throw at the comment") = []
{
    auto options = commentsRejected();

    check(parseErrorMessage("// x\n1", options) == commentsError(0));
    check(parseErrorMessage("/* x */1", options) == commentsError(0));
    check(parseErrorMessage("1 // x", options) == commentsError(2));
    check(parseErrorMessage("1/**/", options) == commentsError(1));
    check(parseErrorMessage("[1, /* x */ 2]", options) == commentsError(4));
    check(parseErrorMessage("[/**/]", options) == commentsError(1));
    check(parseErrorMessage("{/**/}", options) == commentsError(1));
    check(parseErrorMessage(R"({"a" /**/ : 1})", options) == commentsError(5));
    check(parseErrorMessage(R"({"a": //x
1})",
                            options)
          == commentsError(6));
    check(parseErrorMessage("/* never closes", options) == commentsError(0));
};

auto commentsRejectedLeavesTheRest =
    test("ParseOptions: rejecting comments leaves other input alone") = []
{
    auto options = commentsRejected();

    auto strings =
        parseWithOptions(R"({"a":"// not a comment","b":"/* nor */"})", options);
    check(strings["a"].asString() == "// not a comment");
    check(strings["b"].asString() == "/* nor */");
    check(parseWithOptions(" [ 1 , 2 ] ", options).asArray().size() == 2);
    check(parseErrorMessage("/ 1", options)
          == "JSON parse error at position 0: unexpected character '/'");
    check(parseErrorMessage("1 /", options)
          == "JSON parse error at position 2: unexpected character '/'");
};

auto commentsRejectedNonThrowing = test(
    "ParseOptions: rejected comments give null from the non-throwing parse") = []
{
    check(getParsedValueWithOptions("1 // x", commentsRejected()).isNull());
    check(getParsedValueWithOptions("[/**/1]", commentsRejected()).isNull());
    check(getParsedValueWithOptions("[1]", commentsRejected()) == parse("[1]"));
    check(getParsedValue("1 // x").asInteger() == 1);
};

auto generatedCommentsProperty =
    test("ParseOptions: what parses with comments rejected parses alike with "
         "them allowed") = []
{
    auto engine = Engine {7002};
    auto acceptedStrictly = 0;

    for (const auto& value: generatedValues(7003, 300))
    {
        auto text = printWithOptions(value, randomLayout(engine));

        for (auto round = 0; round < 4; ++round)
        {
            auto candidate = round == 0 ? text : mutated(engine, text);
            auto strict = outcomeOf(candidate, commentsRejected());

            if (strict.starts_with("value: "))
            {
                ++acceptedStrictly;
                check(strict == outcomeOf(candidate, ParseOptions {}));
            }
        }
    }

    check(acceptedStrictly >= 300);
};

// --- Last duplicate key wins ---

auto lastDuplicateWins = test("ParseOptions: the last duplicate key wins") = []
{
    auto options = lastKeyWins();

    auto value = parseWithOptions(R"({"a":1,"b":2,"a":"two","a":[3]})", options);
    check(value.asObject().size() == 2);
    check(value["a"].isArray());
    check(value["a"][0].asInteger() == 3);
    check(value["b"].asInteger() == 2);

    auto nested =
        parseWithOptions(R"({"o":{"x":true,"x":false},"o":{"y":1}})", options);
    check(nested["o"].asObject().size() == 1);
    check(nested["o"]["y"].asInteger() == 1);

    auto inner = parseWithOptions(R"([{"x":true,"x":false}])", options);
    check(inner[0]["x"].asBool() == false);

    check(parseWithOptions(R"({"a":1,"a":2})", ParseOptions {})["a"].asInteger()
          == 1);
    check(getParsedValueWithOptions(R"({"a":1,"a":2})", options)["a"].asInteger()
          == 2);
    check(getParsedValueWithOptions(R"({"a":1,"a":})", options).isNull());
};

auto lastDuplicateDecodedKeys =
    test("ParseOptions: duplicates are matched on the decoded key") = []
{
    auto text = R"({"a":1,"a":2,"b\n":3,"b\u000a":4,"é":5,"é":6})";

    auto last = parseWithOptions(text, lastKeyWins());
    check(last.asObject().size() == 3);
    check(last["a"].asInteger() == 2);
    check(last["b\n"].asInteger() == 4);
    check(last["\xc3\xa9"].asInteger() == 6);

    auto first = parse(text);
    check(first.asObject().size() == 3);
    check(first["a"].asInteger() == 1);
    check(first["b\n"].asInteger() == 3);
    check(first["\xc3\xa9"].asInteger() == 5);
};

auto lastDuplicateKeepsKeyOrder =
    test("ParseOptions: last-wins keeps the keys sorted") = []
{
    auto value = parseWithOptions(R"({"c":1,"a":2,"c":3,"b":4})", lastKeyWins());
    check(print(value) == R"({"a":2,"b":4,"c":3})");
};

auto generatedDuplicateProperty =
    test("ParseOptions: last-wins equals first-wins without duplicate keys") = []
{
    auto engine = Engine {7004};

    for (const auto& value: generatedValues(7005, 400))
    {
        auto text = printWithOptions(value, randomLayout(engine));
        check(outcomeOf(text, lastKeyWins()) == outcomeOf(text, ParseOptions {}));
    }
};

auto strictSwitchesAreIndependent =
    test("ParseOptions: comments and duplicate switches are independent") = []
{
    auto both = commentsRejected();
    both.duplicateKeys = ParseOptions::DuplicateKeys::KeepLast;

    check(parseWithOptions(R"({"a":1,"a":2})", both)["a"].asInteger() == 2);
    check(parseErrorMessage(R"({"a":1,/**/"a":2})", both) == commentsError(7));

    check(parseWithOptions(R"({"a":1,/**/"a":2})", lastKeyWins())["a"].asInteger()
          == 2);
    check(parseWithOptions(R"({"a":1,"a":2})", commentsRejected())["a"].asInteger()
          == 1);
};

// --- NUL-terminated input ---

auto nulEndsTheInput = test("ParseOptions: a NUL ends the input") = []
{
    auto options = nulTerminated();

    check(parseWithOptions("1\0"sv, options).asInteger() == 1);
    check(parseWithOptions("{\"a\":[1]}\0"sv, options)["a"][0].asInteger() == 1);
    check(parseWithOptions("[1] \0garbage \0 more"sv, options).asArray().size()
          == 1);
    check(parseWithOptions("\"s\"\0\0\0"sv, options).asString() == "s");
    check(parseWithOptions("{}"sv, options).asObject().empty());
    check(parseWithOptions("1 // tail\0 ]]]"sv, options).asInteger() == 1);
};

auto nulCutsTheDocumentShort =
    test("ParseOptions: a NUL inside the document cuts it short") = []
{
    auto options = nulTerminated();

    check(parseErrorMessage("\0 1"sv, options)
          == "JSON parse error at position 0: unexpected end of input");
    check(parseErrorMessage("[1,\0 2]"sv, options)
          == "JSON parse error at position 3: unexpected end of input");
    check(parseErrorMessage("[1\0]"sv, options)
          == "JSON parse error at position 2: expected ']' but reached end of "
             "input");
    check(parseErrorMessage("\"a\0b\""sv, options)
          == "JSON parse error at position 2: expected '\"' but reached end of "
             "input");
    check(parseErrorMessage("/*\0*/ 1"sv, options)
          == "JSON parse error at position 0: unterminated block comment");
    check(parseErrorMessage("tr\0ue"sv, options)
          == "JSON parse error at position 0: expected 'false'");
};

auto nulTerminatedNonThrowing =
    test("ParseOptions: NUL-terminated through the non-throwing parse") = []
{
    auto options = nulTerminated();

    check(getParsedValueWithOptions("[1,2]\0"sv, options).asArray().size() == 2);
    check(getParsedValueWithOptions("[1,\0 2]"sv, options).isNull());
    check(getParsedValue("[1,2]\0"sv).isNull());
};

auto nulTerminatedWithoutNul =
    test("ParseOptions: NUL-terminated input with no NUL parses as usual") = []
{
    for (auto text: sampleTexts())
    {
        if (text.find('\0') != std::string_view::npos)
            continue;

        check(print(getParsedValueWithOptions(text, nulTerminated()))
              == print(getParsedValue(text)));
        check(parseErrorMessage(text, nulTerminated())
              == parseErrorMessage(text, ParseOptions {}));
    }
};

auto generatedNulProperty =
    test("ParseOptions: NUL-terminated equals parsing the text cut at the NUL") = []
{
    auto engine = Engine {7006};
    auto withNul = 0;

    for (const auto& value: generatedValues(7007, 300))
    {
        auto text = printWithOptions(value, randomLayout(engine));

        for (auto round = 0; round < 4; ++round)
        {
            auto candidate = mutated(engine, text);

            if (round == 0)
                candidate += '\0';

            auto view = std::string_view {candidate};
            withNul += view.find('\0') != std::string_view::npos ? 1 : 0;

            check(outcomeOf(view, nulTerminated())
                  == outcomeOf(view.substr(0, view.find('\0')), ParseOptions {}));
        }
    }

    check(withNul >= 300);
};

auto nulTerminatedWithCommentsRejected =
    test("ParseOptions: NUL-terminated with comments rejected") = []
{
    auto options = nulTerminated();
    options.allowComments = false;

    check(parseWithOptions("[1]\0 // after the end"sv, options).asArray().size()
          == 1);
    check(parseErrorMessage("[1] // c\0"sv, options) == commentsError(4));
    check(getParsedValueWithOptions("[1] // c\0"sv, options).isNull());
};

auto strictRoundTrip =
    test("Options: printed documents parse back under every parse option") = []
{
    auto printOptions = lineBroken();
    printOptions.keepDoublesAsDoubles = true;

    auto parseOptions = commentsRejected();
    parseOptions.duplicateKeys = ParseOptions::DuplicateKeys::KeepLast;
    parseOptions.endAtNul = true;

    auto document = sampleDocument();
    auto text = printWithOptions(document, printOptions);
    text += '\0';

    auto back = parseWithOptions(text, parseOptions);
    check(back == document);
    check(!back["number"].isInteger());
    check(printWithOptions(back, printOptions) + '\0' == text);
};

// --- Locale ---

// Selects a decimal-comma LC_NUMERIC for the scope and puts the previous
// one back on exit; `active` says whether one took.
struct CommaLocale
{
    CommaLocale()
        : previous(std::setlocale(LC_NUMERIC, nullptr))
    {
        for (auto* candidate: {"de_DE.UTF-8",
                               "de_DE.utf8",
                               "de_DE",
                               "de-DE",
                               "German_Germany.1252",
                               "fr_FR.UTF-8",
                               "fr_FR.utf8",
                               "fr_FR",
                               "fr-FR"})
        {
            if (std::setlocale(LC_NUMERIC, candidate) == nullptr)
                continue;

            if (std::string_view(std::localeconv()->decimal_point) == ",")
            {
                active = true;
                return;
            }
        }

        std::setlocale(LC_NUMERIC, previous.c_str());
    }

    ~CommaLocale() { std::setlocale(LC_NUMERIC, previous.c_str()); }

    std::string previous;
    bool active = false;
};

auto keptDoublesUnderDecimalComma =
    test("PrintOptions: kept doubles use '.' under a decimal-comma locale") = []
{
    auto locale = CommaLocale {};

    if (!locale.active)
    {
        std::cout << "  (no decimal-comma locale installed, skipped)\n";
        return;
    }

    auto options = lineBroken();
    options.keepDoublesAsDoubles = true;

    check(printWithOptions(Value {1.0}, options) == "1.0");
    check(printWithOptions(Value {-0.0}, options) == "-0.0");
    check(printWithOptions(Value {1.5}, options) == "1.5");
    check(printWithOptions(Value {1e20}, options) == "1e+20");
    check(printWithOptions(parse("[2.0,0.25]"), options) == "[\n2.0,\n0.25\n]");
    check(bitsOf(parse(printWithOptions(Value {-0.0}, options)).asNumber())
          == bitsOf(-0.0));
};

} // namespace JsonOptionsTests
