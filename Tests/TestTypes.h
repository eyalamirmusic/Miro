#pragma once

#include <Miro/Miro.h>

#include <optional>

struct Inner
{
    void reflect(Miro::Reflector& ref) { ref["x"](x); }

    int x = 0;
};

struct Outer
{
    void reflect(Miro::Reflector& ref)
    {
        ref["a"](a);
        ref["nested"](nested);
        ref["label"](label);
    }

    int a = 0;
    Inner nested;
    std::string label;
};

struct ClassWithBool
{
    void reflect(Miro::Reflector& ref) { ref["active"](active); }

    bool active = true;
};

struct ClassWithInt
{
    void reflect(Miro::Reflector& ref) { ref["count"](count); }

    int count = 42;
};

struct ClassWithDouble
{
    void reflect(Miro::Reflector& ref) { ref["ratio"](ratio); }

    double ratio = 3.14;
};

struct ClassWithString
{
    void reflect(Miro::Reflector& ref) { ref["name"](name); }

    std::string name = "hello";
};

struct ClassWithVectorOfInts
{
    void reflect(Miro::Reflector& ref) { ref["nums"](nums); }

    std::vector<int> nums = {1, 2, 3};
};

struct ClassWithVectorOfObjects
{
    void reflect(Miro::Reflector& ref) { ref["items"](items); }

    std::vector<Inner> items = {{1}, {2}, {3}};
};

struct ClassWithVectorOfStrings
{
    void reflect(Miro::Reflector& ref) { ref["tags"](tags); }

    std::vector<std::string> tags = {"a", "b", "c"};
};

struct ClassWithArrayOfDoubles
{
    void reflect(Miro::Reflector& ref) { ref["vals"](vals); }

    std::array<double, 3> vals = {1.0, 2.0, 3.0};
};

struct ClassWithArrayOfObjects
{
    void reflect(Miro::Reflector& ref) { ref["items"](items); }

    std::array<Inner, 2> items = {Inner {7}, Inner {8}};
};

struct ClassWithStringMap
{
    void reflect(Miro::Reflector& ref) { ref["data"](data); }

    std::map<std::string, std::string> data = {{"a", "hello"}, {"b", "world"}};
};

struct ClassWithIntMap
{
    void reflect(Miro::Reflector& ref) { ref["counts"](counts); }

    std::map<std::string, int> counts = {{"apples", 3}, {"bananas", 5}};
};

struct ClassWithObjectMap
{
    void reflect(Miro::Reflector& ref) { ref["items"](items); }

    std::map<std::string, Inner> items;
};

struct MacroInner
{
    MIRO_REFLECT(x)

    int x = 0;
};

struct MacroReflected
{
    MIRO_REFLECT(name, count, ratio, active, tags, nested)

    std::string name = "macro";
    int count = 7;
    double ratio = 1.5;
    bool active = true;
    std::vector<int> tags = {4, 5, 6};
    MacroInner nested = {42};
};

struct MacroEmpty
{
    MIRO_REFLECT()
};

// Types intentionally without any reflect() method — to be reflected via
// MIRO_REFLECT_EXTERNAL below, simulating a third-party type.
struct ExternalPoint
{
    int x = 3;
    int y = 4;
};

struct ExternalPerson
{
    std::string name = "ext";
    int age = 21;
    ExternalPoint location;
};

struct ExternalEmpty
{
};

struct ExternalWithContainers
{
    std::vector<int> ids = {1, 2, 3};
    std::map<std::string, ExternalPoint> points = {{"a", {1, 2}}, {"b", {3, 4}}};
};

MIRO_REFLECT_EXTERNAL(ExternalPoint, x, y)
MIRO_REFLECT_EXTERNAL(ExternalPerson, name, age, location)
MIRO_REFLECT_EXTERNAL(ExternalEmpty)
MIRO_REFLECT_EXTERNAL(ExternalWithContainers, ids, points)

struct NamedMembers
{
    MIRO_REFLECT_MEMBERS(
        name, "Full Name", count, "Item Count", ratio, "price-ratio")

    std::string name = "named";
    int count = 3;
    double ratio = 0.5;
};

struct NamedMembersEmpty
{
    MIRO_REFLECT_MEMBERS()
};

struct ExternalNamed
{
    double price = 9.99;
    bool inStock = true;
};

MIRO_REFLECT_EXTERNAL_MEMBERS(
    ExternalNamed, price, "Unit Price", inStock, "In Stock")

struct ClassWithIntegrals
{
    void reflect(Miro::Reflector& ref)
    {
        ref["u"](u);
        ref["s"](s);
        ref["ll"](ll);
        ref["c"](c);
    }

    unsigned int u = 5;
    short s = -3;
    long long ll = 1234567890123LL;
    char c = 'A';
};

struct ClassWithOptional
{
    void reflect(Miro::Reflector& ref)
    {
        ref["maybeInt"](maybeInt);
        ref["maybeInner"](maybeInner);
    }

    std::optional<int> maybeInt;
    std::optional<Inner> maybeInner;
};

enum class Color
{
    Red,
    Green,
    Blue
};

enum class Signal : int
{
    Stop = -1,
    Go = 1,
    Wait = 2
};

enum UnscopedMode : int
{
    ModeOff = 0,
    ModeOn = 1,
    ModeAuto = 2
};

struct ClassWithEnum
{
    MIRO_REFLECT(color, signal, mode)

    Color color = Color::Green;
    Signal signal = Signal::Go;
    UnscopedMode mode = ModeAuto;
};
