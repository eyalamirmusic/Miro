#include <Miro/Json.h>
#include <NanoTest/NanoTest.h>

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
    options.duplicateKeys = ParseOptions::DuplicateKeys::keepLast;
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
        parse(text, options);
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

// --- Defaults ---

auto defaultPrintOptions =
    test("PrintOptions: defaults print exactly as print(value, n)") = []
{
    for (const auto& value: sampleValues())
    {
        check(print(value, PrintOptions {}) == print(value));

        for (auto indent: {-3, -1, 0, 1, 2, 4, 8})
            check(print(value, indented(indent)) == print(value, indent));
    }

    for (auto number: {1.0, -0.0, 0.0, 1e15, 1e20, 1e300, 0.1, 5e-324})
        check(print(Value {number}, PrintOptions {}) == print(Value {number}));
};

auto defaultParseOptions =
    test("ParseOptions: defaults parse exactly as parse(text)") = []
{
    for (auto text: sampleTexts())
    {
        auto plain = getParsedValue(text);
        auto withOptions = getParsedValue(text, ParseOptions {});

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

    check(print(Value {1.0}, options) == "1.0");
    check(print(Value {0.0}, options) == "0.0");
    check(print(Value {-0.0}, options) == "-0.0");
    check(print(Value {-3.0}, options) == "-3.0");
    check(print(Value {123456789.0}, options) == "123456789.0");
    check(print(Value {1e15}, options) == "1e+15");
    check(print(Value {1e20}, options) == "1e+20");
    check(print(Value {-1e20}, options) == "-1e+20");
    check(print(Value {1e300}, options) == "1e+300");
    check(print(Value {9007199254740992.0}, options) == "9007199254740992.0");
    check(print(Value {std::numeric_limits<double>::max()}, options)
          == "1.7976931348623157e+308");
};

auto keptDoublesParseBackAsDoubles =
    test("PrintOptions: kept doubles parse back as the same double") = []
{
    auto options = doublesKept();

    for (auto number: {1.0, 0.0, -0.0, -3.0, 1e15, 1e20, 1e300, 4503599627370496.0})
    {
        auto back = parse(print(Value {number}, options));

        check(!back.isInteger());
        check(bitsOf(back.asNumber()) == bitsOf(number));
    }
};

auto keptDoublesLeaveTheRestAlone =
    test("PrintOptions: integers, non-integral and non-finite unaffected") = []
{
    auto options = doublesKept();

    check(print(Value {1}, options) == "1");
    check(print(Value {0}, options) == "0");
    check(print(parse("-0"), options) == "0");
    check(print(Value {std::numeric_limits<std::int64_t>::min()}, options)
          == "-9223372036854775808");
    check(print(Value {std::numeric_limits<std::int64_t>::max()}, options)
          == "9223372036854775807");

    check(print(Value {std::numeric_limits<double>::quiet_NaN()}, options)
          == "null");
    check(print(Value {std::numeric_limits<double>::infinity()}, options) == "null");
    check(print(Value {-std::numeric_limits<double>::infinity()}, options)
          == "null");

    for (auto number: {0.1, 1.0 / 3.0, 123456.789, 0.1 + 0.2, 5e-324, 1e-7, -2.5})
        check(print(Value {number}, options) == print(Value {number}));
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

    auto printed = print(Value {object}, doublesKept());
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

        auto token = print(Value {number}, options);
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

    check(print(value, lineBroken()) == expected);
};

auto lineBrokenEmptiesAndScalars =
    test("PrintOptions: line-broken empties and scalars") = []
{
    auto options = lineBroken();

    check(print(parse("[]"), options) == "[]");
    check(print(parse("{}"), options) == "{}");
    check(print(parse("[[]]"), options) == "[\n[]\n]");
    check(print(parse(R"({"a":{}})"), options) == "{\n\"a\": {}\n}");
    check(print(Value {42}, options) == "42");
    check(print(Value {"s"}, options) == "\"s\"");
    check(print(Value {}, options) == "null");
};

auto lineBrokenIsIndentedWithoutSpaces =
    test("PrintOptions: line-broken equals indent n without leading spaces") = []
{
    for (const auto& value: sampleValues())
    {
        auto broken = print(value, lineBroken());

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
            check(print(value, options) == print(value, indent));
        }

        for (auto indent: {0, -1, -4})
        {
            auto options = indented(indent);
            options.breakLines = true;
            check(print(value, options) == print(value, lineBroken()));
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

    check(print(value, options) == expected);

    auto indentedKept = indented(4);
    indentedKept.keepDoublesAsDoubles = true;
    check(print(value, options) == withoutLeadingSpaces(print(value, indentedKept)));
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

                auto back = parse(print(document, options));
                check(back == document);
                check(print(back, options) == print(document, options));
                check(back["number"].isInteger() == !keepDoubles);
                check(std::signbit(back["zero"].asNumber()) == keepDoubles);
            }
        }
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

    auto strings = parse(R"({"a":"// not a comment","b":"/* nor */"})", options);
    check(strings["a"].asString() == "// not a comment");
    check(strings["b"].asString() == "/* nor */");
    check(parse(" [ 1 , 2 ] ", options).asArray().size() == 2);
    check(parseErrorMessage("/ 1", options)
          == "JSON parse error at position 0: unexpected character '/'");
    check(parseErrorMessage("1 /", options)
          == "JSON parse error at position 2: unexpected character '/'");
};

auto commentsRejectedNonThrowing =
    test("ParseOptions: rejected comments give null from getParsedValue") = []
{
    check(getParsedValue("1 // x", commentsRejected()).isNull());
    check(getParsedValue("[/**/1]", commentsRejected()).isNull());
    check(getParsedValue("[1]", commentsRejected()) == parse("[1]"));
    check(getParsedValue("1 // x").asInteger() == 1);
};

// --- Last duplicate key wins ---

auto lastDuplicateWins = test("ParseOptions: the last duplicate key wins") = []
{
    auto options = lastKeyWins();

    auto value = parse(R"({"a":1,"b":2,"a":"two","a":[3]})", options);
    check(value.asObject().size() == 2);
    check(value["a"].isArray());
    check(value["a"][0].asInteger() == 3);
    check(value["b"].asInteger() == 2);

    auto nested = parse(R"({"o":{"x":true,"x":false},"o":{"y":1}})", options);
    check(nested["o"].asObject().size() == 1);
    check(nested["o"]["y"].asInteger() == 1);

    auto inner = parse(R"([{"x":true,"x":false}])", options);
    check(inner[0]["x"].asBool() == false);

    check(parse(R"({"a":1,"a":2})", ParseOptions {})["a"].asInteger() == 1);
    check(getParsedValue(R"({"a":1,"a":2})", options)["a"].asInteger() == 2);
    check(getParsedValue(R"({"a":1,"a":})", options).isNull());
};

auto lastDuplicateKeepsKeyOrder =
    test("ParseOptions: last-wins keeps the keys sorted") = []
{
    auto value = parse(R"({"c":1,"a":2,"c":3,"b":4})", lastKeyWins());
    check(print(value) == R"({"a":2,"b":4,"c":3})");
};

auto strictSwitchesAreIndependent =
    test("ParseOptions: comments and duplicate switches are independent") = []
{
    auto both = commentsRejected();
    both.duplicateKeys = ParseOptions::DuplicateKeys::keepLast;

    check(parse(R"({"a":1,"a":2})", both)["a"].asInteger() == 2);
    check(parseErrorMessage(R"({"a":1,/**/"a":2})", both) == commentsError(7));

    check(parse(R"({"a":1,/**/"a":2})", lastKeyWins())["a"].asInteger() == 2);
    check(parse(R"({"a":1,"a":2})", commentsRejected())["a"].asInteger() == 1);
};

// --- NUL-terminated input ---

auto nulEndsTheInput = test("ParseOptions: a NUL ends the input") = []
{
    auto options = nulTerminated();

    check(parse("1\0"sv, options).asInteger() == 1);
    check(parse("{\"a\":[1]}\0"sv, options)["a"][0].asInteger() == 1);
    check(parse("[1] \0garbage \0 more"sv, options).asArray().size() == 1);
    check(parse("\"s\"\0\0\0"sv, options).asString() == "s");
    check(parse("{}"sv, options).asObject().empty());
    check(parse("1 // tail\0 ]]]"sv, options).asInteger() == 1);
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
    test("ParseOptions: NUL-terminated through getParsedValue") = []
{
    auto options = nulTerminated();

    check(getParsedValue("[1,2]\0"sv, options).asArray().size() == 2);
    check(getParsedValue("[1,\0 2]"sv, options).isNull());
    check(getParsedValue("[1,2]\0"sv).isNull());
};

auto nulTerminatedWithoutNul =
    test("ParseOptions: NUL-terminated input with no NUL parses as usual") = []
{
    for (auto text: sampleTexts())
    {
        if (text.find('\0') != std::string_view::npos)
            continue;

        check(print(getParsedValue(text, nulTerminated()))
              == print(getParsedValue(text)));
        check(parseErrorMessage(text, nulTerminated())
              == parseErrorMessage(text, ParseOptions {}));
    }
};

auto nulTerminatedWithCommentsRejected =
    test("ParseOptions: NUL-terminated with comments rejected") = []
{
    auto options = nulTerminated();
    options.allowComments = false;

    check(parse("[1]\0 // after the end"sv, options).asArray().size() == 1);
    check(parseErrorMessage("[1] // c\0"sv, options) == commentsError(4));
    check(getParsedValue("[1] // c\0"sv, options).isNull());
};

auto strictRoundTrip =
    test("Options: printed documents parse back under every parse option") = []
{
    auto printOptions = lineBroken();
    printOptions.keepDoublesAsDoubles = true;

    auto parseOptions = commentsRejected();
    parseOptions.duplicateKeys = ParseOptions::DuplicateKeys::keepLast;
    parseOptions.endAtNul = true;

    auto document = sampleDocument();
    auto text = print(document, printOptions);
    text += '\0';

    auto back = parse(text, parseOptions);
    check(back == document);
    check(!back["number"].isInteger());
    check(print(back, printOptions) + '\0' == text);
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

    check(print(Value {1.0}, options) == "1.0");
    check(print(Value {-0.0}, options) == "-0.0");
    check(print(Value {1.5}, options) == "1.5");
    check(print(Value {1e20}, options) == "1e+20");
    check(print(parse("[2.0,0.25]"), options) == "[\n2.0,\n0.25\n]");
    check(bitsOf(parse(print(Value {-0.0}, options)).asNumber()) == bitsOf(-0.0));
};

} // namespace JsonOptionsTests
