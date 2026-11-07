#include <Miro/Reflection/PrimitiveString.h>

#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

auto primitiveIntRoundTrip = test("PrimitiveString: int to and from text") = []
{
    auto value = 42;
    check(primitiveToString(&value) == "42");

    auto parsed = 0;
    primitiveFromString("-17", &parsed);
    check(parsed == -17);
};

auto primitiveDoubleRoundTrip = test("PrimitiveString: double to and from text") = []
{
    auto whole = 5.0;
    check(primitiveToString(&whole) == "5");

    auto fraction = 2.5;
    check(primitiveToString(&fraction) == "2.5");

    auto parsed = 0.0;
    primitiveFromString("3.25", &parsed);
    check(parsed == 3.25);
};

auto primitiveBoolRoundTrip = test("PrimitiveString: bool to and from text") = []
{
    auto value = true;
    check(primitiveToString(&value) == "true");

    auto parsed = false;
    primitiveFromString("1", &parsed);
    check(parsed);
    primitiveFromString("false", &parsed);
    check(!parsed);
};

auto primitiveStringRoundTrip = test("PrimitiveString: string to and from text") = []
{
    auto value = std::string {"hello world"};
    check(primitiveToString(&value) == "hello world");

    auto parsed = std::string {};
    primitiveFromString("a b c", &parsed);
    check(parsed == "a b c");
};
