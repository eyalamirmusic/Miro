#pragma once

#include <Miro/Miro.h>

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
