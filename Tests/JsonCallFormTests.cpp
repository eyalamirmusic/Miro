// Compiles every way callers spell the JSON layer's parse and print
// entry points, so an added overload that makes one of them ambiguous
// fails the build here. The checks are a smoke; compiling is the test.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <string>
#include <string_view>

using namespace nano;

namespace JsonCallFormTests
{

struct Point
{
    void reflect(Miro::Reflector& ref)
    {
        ref["x"](x);
        ref["y"](y);
    }

    int x = 0;
    int y = 0;
};

std::string makeText()
{
    return R"({"x":1,"y":2})";
}

auto parseCallForms = test("Call forms: parse") = []
{
    const auto lvalue = std::string {R"({"x":1})"};
    auto view = std::string_view {lvalue};

    check(Miro::Json::parse(lvalue)["x"].asInteger() == 1);
    check(Miro::Json::parse(std::string(view))["x"].asInteger() == 1);
    check(Miro::Json::parse(makeText())["y"].asInteger() == 2);
    check(Miro::Json::parse(R"({"x":1})")["x"].asInteger() == 1);
    check(Miro::Json::parse(view)["x"].asInteger() == 1);
    check(Miro::Json::parse(lvalue.substr(0))["x"].asInteger() == 1);
};

auto getParsedValueCallForms = test("Call forms: getParsedValue") = []
{
    const auto lvalue = std::string {R"({"x":1})"};
    auto view = std::string_view {lvalue};

    check(Miro::Json::getParsedValue(lvalue)["x"].asInteger() == 1);
    check(Miro::Json::getParsedValue(view)["x"].asInteger() == 1);
    check(Miro::Json::getParsedValue(R"({"x":1})")["x"].asInteger() == 1);
    check(Miro::Json::getParsedValue(makeText())["y"].asInteger() == 2);
};

auto printCallForms = test("Call forms: print") = []
{
    const auto value = Miro::Json::parse(R"({"x":[1]})");
    const auto* pointer = &value;
    auto indent = 2;

    check(Miro::Json::print(value) == R"({"x":[1]})");
    check(Miro::Json::print(*pointer) == R"({"x":[1]})");
    check(Miro::Json::print(Miro::JSON {"key"}) == R"("key")");
    check(Miro::Json::print(value, 2) == "{\n  \"x\": [\n    1\n  ]\n}");
    check(Miro::Json::print(value, 4) == "{\n    \"x\": [\n        1\n    ]\n}");
    check(Miro::Json::print(value, indent) == Miro::Json::print(value, 2));
    check(Miro::Json::print(value, 0) == R"({"x":[1]})");
    check(Miro::Json::print(value, {}) == R"({"x":[1]})");
};

auto unqualifiedCallForms = test("Call forms: unqualified through ADL") = []
{
    auto value = Miro::Json::parse("[1]");

    check(print(value) == "[1]");
    check(print(value, 1) == "[\n 1\n]");
};

auto reflectionCallForms = test("Call forms: composed with reflection") = []
{
    auto point = Point {};
    Miro::fromJSON(point, Miro::Json::parse(makeText()));
    check(point.x == 1 && point.y == 2);

    auto created = Miro::createFromJSON<Point>(Miro::Json::parse(makeText()));
    check(created.y == 2);

    auto fromSafe = Miro::createFromJSON<Point>(Miro::Json::getParsedValue("{}"));
    check(fromSafe.x == 0);

    check(Miro::Json::print(Miro::toJSON(point)) == R"({"x":1,"y":2})");
};

} // namespace JsonCallFormTests
