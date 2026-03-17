#include <Miro/JsonLoader.h>
#include <Miro/JsonSaver.h>
#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro::Json;

// --- Test types ---

struct Inner
{
    int x = 0;

    void reflect(Miro::Reflector& ref) { ref["x"](x); }
};

struct Outer
{
    int a = 0;
    Inner nested;
    std::string label;

    void reflect(Miro::Reflector& ref)
    {
        ref["a"](a);
        ref["nested"](nested);
        ref["label"](label);
    }
};

// --- Save tests ---

auto saveBool = test("Save bool") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["active"](active); }

        bool active = true;
    };

    auto saver = Saver {};
    auto val = S {};
    val.reflect(saver);

    check(saver.root["active"].isBool());
    check(saver.root["active"].asBool() == true);
};

auto saveInt = test("Save int") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["count"](count); }

        int count = 42;
    };

    auto saver = Saver {};
    auto val = S {};
    val.reflect(saver);

    check(saver.root["count"].isNumber());
    check(saver.root["count"].asNumber() == 42.0);
};

auto saveDouble = test("Save double") = []
{
    struct S
    {
        double ratio = 3.14;
        void reflect(Miro::Reflector& ref) { ref["ratio"](ratio); }
    };

    auto saver = Saver {};
    auto val = S {};
    val.reflect(saver);

    check(saver.root["ratio"].isNumber());
    check(saver.root["ratio"].asNumber() == 3.14);
};

auto saveString = test("Save string") = []
{
    struct S
    {
        std::string name = "hello";
        void reflect(Miro::Reflector& ref) { ref["name"](name); }
    };

    auto saver = Saver {};
    auto val = S {};
    val.reflect(saver);

    check(saver.root["name"].isString());
    check(saver.root["name"].asString() == "hello");
};

// --- Load tests ---

auto loadBool = test("Load bool") = []
{
    struct S
    {
        bool active = false;
        void reflect(Miro::Reflector& ref) { ref["active"](active); }
    };

    auto json = parse(R"({"active": true})");
    auto loader = Loader {json};
    auto val = S {};
    val.reflect(loader);

    check(val.active == true);
};

auto loadInt = test("Load int") = []
{
    struct S
    {
        int count = 0;
        void reflect(Miro::Reflector& ref) { ref["count"](count); }
    };

    auto json = parse(R"({"count": 7})");
    auto loader = Loader {json};
    auto val = S {};
    val.reflect(loader);

    check(val.count == 7);
};

auto loadDouble = test("Load double") = []
{
    struct S
    {
        double ratio = 0.0;
        void reflect(Miro::Reflector& ref) { ref["ratio"](ratio); }
    };

    auto json = parse(R"({"ratio": 2.72})");
    auto loader = Loader {json};
    auto val = S {};
    val.reflect(loader);

    check(val.ratio == 2.72);
};

auto loadString = test("Load string") = []
{
    struct S
    {
        std::string name;
        void reflect(Miro::Reflector& ref) { ref["name"](name); }
    };

    auto json = parse(R"({"name": "world"})");
    auto loader = Loader {json};
    auto val = S {};
    val.reflect(loader);

    check(val.name == "world");
};

// --- Object tests ---

auto saveObject = test("Save object") = []
{
    auto saver = Saver {};
    auto val = Inner {5};
    val.reflect(saver);

    check(saver.root["x"].isNumber());
    check(saver.root["x"].asNumber() == 5.0);
};

auto loadObject = test("Load object") = []
{
    auto json = parse(R"({"x": 99})");
    auto loader = Loader {json};
    auto val = Inner {};
    val.reflect(loader);

    check(val.x == 99);
};

// --- Nested object tests ---

auto saveNested = test("Save nested objects") = []
{
    auto saver = Saver {};
    auto val = Outer {10, {20}, "test"};
    val.reflect(saver);

    check(saver.root["a"].asNumber() == 10.0);
    check(saver.root["nested"]["x"].asNumber() == 20.0);
    check(saver.root["label"].asString() == "test");
};

auto loadNested = test("Load nested objects") = []
{
    auto json = parse(R"({"a": 77, "nested": {"x": 88}, "label": "loaded"})");
    auto loader = Loader {json};
    auto val = Outer {};
    val.reflect(loader);

    check(val.a == 77);
    check(val.nested.x == 88);
    check(val.label == "loaded");
};

// --- Roundtrip test ---

auto roundtrip = test("Save then load roundtrip") = []
{
    auto original = Outer {10, {20}, "hello"};

    auto saver = Saver {};
    original.reflect(saver);

    auto loaded = Outer {};
    auto loader = Loader {saver.root};
    loaded.reflect(loader);

    check(loaded.a == original.a);
    check(loaded.nested.x == original.nested.x);
    check(loaded.label == original.label);
};
