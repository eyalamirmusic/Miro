#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

using namespace nano;

// --- Save tests ---

auto saveBool = test("Save bool") = []
{
    auto val = ClassWithBool {};
    auto json = Miro::toJSON(val);

    check(json["active"].isBool());
    check(json["active"].asBool() == true);
};

auto saveInt = test("Save int") = []
{
    auto val = ClassWithInt {};
    auto json = Miro::toJSON(val);

    check(json["count"].isNumber());
    check(json["count"].asNumber() == 42.0);
};

auto saveDouble = test("Save double") = []
{
    auto val = ClassWithDouble {};
    auto json = Miro::toJSON(val);

    check(json["ratio"].isNumber());
    check(json["ratio"].asNumber() == 3.14);
};

auto saveString = test("Save string") = []
{
    auto val = ClassWithString {};
    auto json = Miro::toJSON(val);

    check(json["name"].isString());
    check(json["name"].asString() == "hello");
};

// --- Load tests ---

auto loadBool = test("Load bool") = []
{
    auto val = Miro::createFromJSONString<ClassWithBool>(R"({"active": true})");

    check(val.active == true);
};

auto loadInt = test("Load int") = []
{
    auto val = Miro::createFromJSONString<ClassWithInt>(R"({"count": 7})");

    check(val.count == 7);
};

auto loadDouble = test("Load double") = []
{
    auto val = Miro::createFromJSONString<ClassWithDouble>(R"({"ratio": 2.72})");

    check(val.ratio == 2.72);
};

auto loadString = test("Load string") = []
{
    auto val = Miro::createFromJSONString<ClassWithString>(R"({"name": "world"})");

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
    auto val = ClassWithVectorOfInts {};
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
    auto val = Miro::createFromJSONString<ClassWithVectorOfInts>(
        R"({"nums": [10, 20, 30]})");

    check(val.nums.size() == 3);
    check(val.nums[0] == 10);
    check(val.nums[1] == 20);
    check(val.nums[2] == 30);
};

auto saveVectorOfObjects = test("Save vector of objects") = []
{
    auto val = ClassWithVectorOfObjects {};
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
    auto val = Miro::createFromJSONString<ClassWithVectorOfObjects>(
        R"({"items": [{"x": 5}, {"x": 10}]})");

    check(val.items.size() == 2);
    check(val.items[0].x == 5);
    check(val.items[1].x == 10);
};

auto vectorRoundtrip = test("Vector roundtrip") = []
{
    auto original = ClassWithVectorOfStrings {};
    auto json = Miro::toJSON(original);
    auto loaded = Miro::createFromJSON<ClassWithVectorOfStrings>(json);

    check(loaded.tags.size() == 3);
    check(loaded.tags[0] == "a");
    check(loaded.tags[1] == "b");
    check(loaded.tags[2] == "c");
};

// --- std::array tests ---

auto saveArrayOfDoubles = test("Save array of doubles") = []
{
    auto val = ClassWithArrayOfDoubles {};
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
    auto val = Miro::createFromJSONString<ClassWithArrayOfDoubles>(
        R"({"vals": [4.0, 5.0, 6.0]})");

    check(val.vals[0] == 4.0);
    check(val.vals[1] == 5.0);
    check(val.vals[2] == 6.0);
};

auto saveArrayOfObjects = test("Save array of objects") = []
{
    auto val = ClassWithArrayOfObjects {};
    auto json = Miro::toJSON(val);

    check(json["items"].isArray());
    auto& arr = json["items"].asArray();
    check(arr.size() == 2);
    check(arr[0]["x"].asNumber() == 7.0);
    check(arr[1]["x"].asNumber() == 8.0);
};

auto loadArrayOfObjects = test("Load array of objects") = []
{
    auto val = Miro::createFromJSONString<ClassWithArrayOfObjects>(
        R"({"items": [{"x": 11}, {"x": 22}]})");

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

// --- Map tests ---

auto saveMapOfStrings = test("Save map of strings") = []
{
    auto val = ClassWithStringMap {};
    auto json = Miro::toJSON(val);

    check(json["data"].isObject());
    check(json["data"]["a"].asString() == "hello");
    check(json["data"]["b"].asString() == "world");
};

auto loadMapOfStrings = test("Load map of strings") = []
{
    auto val = Miro::createFromJSONString<ClassWithStringMap>(
        R"({"data": {"x": "one", "y": "two"}})");

    check(val.data.size() == 2);
    check(val.data["x"] == "one");
    check(val.data["y"] == "two");
};

auto saveMapOfInts = test("Save map of ints") = []
{
    auto val = ClassWithIntMap {};
    auto json = Miro::toJSON(val);

    check(json["counts"]["apples"].asNumber() == 3.0);
    check(json["counts"]["bananas"].asNumber() == 5.0);
};

auto loadMapOfObjects = test("Load map of objects") = []
{
    auto val = Miro::createFromJSONString<ClassWithObjectMap>(
        R"({"items": {"first": {"x": 1}, "second": {"x": 2}}})");

    check(val.items.size() == 2);
    check(val.items["first"].x == 1);
    check(val.items["second"].x == 2);
};

auto mapRoundtrip = test("Map roundtrip") = []
{
    auto original = ClassWithIntMap {};
    auto json = Miro::toJSON(original);
    auto loaded = Miro::createFromJSON<ClassWithIntMap>(json);

    check(loaded.counts.size() == 2);
    check(loaded.counts["apples"] == 3);
    check(loaded.counts["bananas"] == 5);
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

// --- Missing property tests ---

auto missingPropertyKeepsDefault = test("Missing property keeps existing value") = []
{
    auto val = Outer {10, {20}, "original"};
    Miro::fromJSONString(val, R"({"a": 99})");

    check(val.a == 99);
    check(val.nested.x == 20);
    check(val.label == "original");
};

// --- MIRO_REFLECT macro tests ---

auto saveMacroReflected = test("Save MIRO_REFLECT struct") = []
{
    auto val = MacroReflected {};
    auto json = Miro::toJSON(val);

    check(json["name"].asString() == "macro");
    check(json["count"].asNumber() == 7.0);
    check(json["ratio"].asNumber() == 1.5);
    check(json["active"].asBool() == true);
    check(json["tags"].isArray());
    check(json["tags"][0].asNumber() == 4.0);
    check(json["nested"]["x"].asNumber() == 42.0);
};

auto loadMacroReflected = test("Load MIRO_REFLECT struct") = []
{
    auto val = Miro::createFromJSONString<MacroReflected>(
        R"({
            "name": "loaded",
            "count": 99,
            "ratio": 2.5,
            "active": false,
            "tags": [1, 2],
            "nested": {"x": 11}
        })");

    check(val.name == "loaded");
    check(val.count == 99);
    check(val.ratio == 2.5);
    check(val.active == false);
    check(val.tags.size() == 2);
    check(val.tags[0] == 1);
    check(val.nested.x == 11);
};

auto macroReflectedRoundtrip = test("MIRO_REFLECT roundtrip") = []
{
    auto original = MacroReflected {"rt", 3, 0.25, false, {7, 8}, {55}};
    auto loaded = Miro::createFromJSON<MacroReflected>(Miro::toJSON(original));

    check(loaded.name == original.name);
    check(loaded.count == original.count);
    check(loaded.ratio == original.ratio);
    check(loaded.active == original.active);
    check(loaded.tags == original.tags);
    check(loaded.nested.x == original.nested.x);
};

auto macroEmpty = test("MIRO_REFLECT with no fields") = []
{
    auto val = MacroEmpty {};
    auto json = Miro::toJSON(val);

    check(json.isObject());
    check(json.asObject().empty());
};
