#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

using namespace nano;

// --- Test types ---

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

// --- Save tests ---

auto saveBool = test("Save bool") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["active"](active); }

        bool active = true;
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["active"].isBool());
    check(json["active"].asBool() == true);
};

auto saveInt = test("Save int") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["count"](count); }

        int count = 42;
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["count"].isNumber());
    check(json["count"].asNumber() == 42.0);
};

auto saveDouble = test("Save double") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["ratio"](ratio); }

        double ratio = 3.14;
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["ratio"].isNumber());
    check(json["ratio"].asNumber() == 3.14);
};

auto saveString = test("Save string") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["name"](name); }

        std::string name = "hello";
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["name"].isString());
    check(json["name"].asString() == "hello");
};

// --- Load tests ---

auto loadBool = test("Load bool") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["active"](active); }

        bool active = false;
    };

    auto val = Miro::createFromJSONString<S>(R"({"active": true})");

    check(val.active == true);
};

auto loadInt = test("Load int") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["count"](count); }

        int count = 0;
    };

    auto val = Miro::createFromJSONString<S>(R"({"count": 7})");

    check(val.count == 7);
};

auto loadDouble = test("Load double") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["ratio"](ratio); }

        double ratio = 0.0;
    };

    auto val = Miro::createFromJSONString<S>(R"({"ratio": 2.72})");

    check(val.ratio == 2.72);
};

auto loadString = test("Load string") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["name"](name); }

        std::string name;
    };

    auto val = Miro::createFromJSONString<S>(R"({"name": "world"})");

    check(val.name == "world");
};

// --- Object tests ---

auto saveObject = test("Save object") = []
{
    auto val = Inner {5};
    auto json = Miro::toJSON(val);

    check(json["x"].isNumber());
    check(json["x"].asNumber() == 5.0);
};

auto loadObject = test("Load object") = []
{
    auto val = Miro::createFromJSONString<Inner>(R"({"x": 99})");

    check(val.x == 99);
};

// --- Nested object tests ---

auto saveNested = test("Save nested objects") = []
{
    auto val = Outer {10, {20}, "test"};
    auto json = Miro::toJSON(val);

    check(json["a"].asNumber() == 10.0);
    check(json["nested"]["x"].asNumber() == 20.0);
    check(json["label"].asString() == "test");
};

auto loadNested = test("Load nested objects") = []
{
    auto val = Miro::createFromJSONString<Outer>(
        R"({"a": 77, "nested": {"x": 88}, "label": "loaded"})");

    check(val.a == 77);
    check(val.nested.x == 88);
    check(val.label == "loaded");
};

// --- Vector tests ---

auto saveVectorOfInts = test("Save vector of ints") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["nums"](nums); }

        std::vector<int> nums = {1, 2, 3};
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["nums"].isArray());
    auto& arr = json["nums"].asArray();
    check(arr.size() == 3);
    check(arr[0].asNumber() == 1.0);
    check(arr[1].asNumber() == 2.0);
    check(arr[2].asNumber() == 3.0);
};

auto loadVectorOfInts = test("Load vector of ints") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["nums"](nums); }

        std::vector<int> nums;
    };

    auto val = Miro::createFromJSONString<S>(R"({"nums": [10, 20, 30]})");

    check(val.nums.size() == 3);
    check(val.nums[0] == 10);
    check(val.nums[1] == 20);
    check(val.nums[2] == 30);
};

auto saveVectorOfObjects = test("Save vector of objects") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["items"](items); }

        std::vector<Inner> items = {{1}, {2}, {3}};
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["items"].isArray());
    auto& arr = json["items"].asArray();
    check(arr.size() == 3);
    check(arr[0]["x"].asNumber() == 1.0);
    check(arr[1]["x"].asNumber() == 2.0);
    check(arr[2]["x"].asNumber() == 3.0);
};

auto loadVectorOfObjects = test("Load vector of objects") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["items"](items); }

        std::vector<Inner> items;
    };

    auto val = Miro::createFromJSONString<S>(R"({"items": [{"x": 5}, {"x": 10}]})");

    check(val.items.size() == 2);
    check(val.items[0].x == 5);
    check(val.items[1].x == 10);
};

auto vectorRoundtrip = test("Vector roundtrip") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["tags"](tags); }

        std::vector<std::string> tags = {"a", "b", "c"};
    };

    auto original = S {};
    auto json = Miro::toJSON(original);
    auto loaded = Miro::createFromJSON<S>(json);

    check(loaded.tags.size() == 3);
    check(loaded.tags[0] == "a");
    check(loaded.tags[1] == "b");
    check(loaded.tags[2] == "c");
};

// --- std::array tests ---

auto saveArrayOfDoubles = test("Save array of doubles") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["vals"](vals); }

        std::array<double, 3> vals = {1.0, 2.0, 3.0};
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["vals"].isArray());
    auto& arr = json["vals"].asArray();
    check(arr.size() == 3);
    check(arr[0].asNumber() == 1.0);
    check(arr[1].asNumber() == 2.0);
    check(arr[2].asNumber() == 3.0);
};

auto loadArrayOfDoubles = test("Load array of doubles") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["vals"](vals); }

        std::array<double, 3> vals = {};
    };

    auto val = Miro::createFromJSONString<S>(R"({"vals": [4.0, 5.0, 6.0]})");

    check(val.vals[0] == 4.0);
    check(val.vals[1] == 5.0);
    check(val.vals[2] == 6.0);
};

auto saveArrayOfObjects = test("Save array of objects") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["items"](items); }

        std::array<Inner, 2> items = {Inner {7}, Inner {8}};
    };

    auto val = S {};
    auto json = Miro::toJSON(val);

    check(json["items"].isArray());
    auto& arr = json["items"].asArray();
    check(arr.size() == 2);
    check(arr[0]["x"].asNumber() == 7.0);
    check(arr[1]["x"].asNumber() == 8.0);
};

auto loadArrayOfObjects = test("Load array of objects") = []
{
    struct S
    {
        void reflect(Miro::Reflector& ref) { ref["items"](items); }

        std::array<Inner, 2> items = {};
    };

    auto val = Miro::createFromJSONString<S>(R"({"items": [{"x": 11}, {"x": 22}]})");

    check(val.items[0].x == 11);
    check(val.items[1].x == 22);
};

// --- Roundtrip test ---

auto roundtrip = test("Save then load roundtrip") = []
{
    auto original = Outer {10, {20}, "hello"};
    auto json = Miro::toJSON(original);
    auto loaded = Miro::createFromJSON<Outer>(json);

    check(loaded.a == original.a);
    check(loaded.nested.x == original.nested.x);
    check(loaded.label == original.label);
};

// --- fromJSON test ---

auto fromJSONTest = test("fromJSON into existing object") = []
{
    auto val = Inner {99};
    Miro::fromJSONString(val, R"({"x": 42})");

    check(val.x == 42);
};

// --- JSON string tests ---

auto toJSONStringTest = test("toJSONString") = []
{
    auto val = Inner {5};
    auto loaded = Miro::createFromJSONString<Inner>(Miro::toJSONString(val));

    check(loaded.x == 5);
};

auto fromJSONStringTest = test("fromJSONString") = []
{
    auto val = Inner {};
    Miro::fromJSONString(val, R"({"x": 77})");

    check(val.x == 77);
};

auto createFromJSONStringTest = test("createFromJSONString") = []
{
    auto val = Miro::createFromJSONString<Outer>(
        R"({"a": 1, "nested": {"x": 2}, "label": "hi"})");

    check(val.a == 1);
    check(val.nested.x == 2);
    check(val.label == "hi");
};
