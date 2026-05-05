#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

// --- Save tests ---

auto saveBool = test("Save bool") = []
{
    auto val = ClassWithBool {};
    auto json = toJSON(val);

    check(json["active"].isBool());
    check(json["active"].asBool() == true);
};

auto saveInt = test("Save int") = []
{
    auto val = ClassWithInt {};
    auto json = toJSON(val);

    check(json["count"].isNumber());
    check(json["count"].asNumber() == 42.0);
};

auto saveDouble = test("Save double") = []
{
    auto val = ClassWithDouble {};
    auto json = toJSON(val);

    check(json["ratio"].isNumber());
    check(json["ratio"].asNumber() == 3.14);
};

auto saveString = test("Save string") = []
{
    auto val = ClassWithString {};
    auto json = toJSON(val);

    check(json["name"].isString());
    check(json["name"].asString() == "hello");
};

// --- Load tests ---

auto loadBool = test("Load bool") = []
{
    auto val = createFromJSONString<ClassWithBool>(R"({"active": true})");

    check(val.active == true);
};

auto loadInt = test("Load int") = []
{
    auto val = createFromJSONString<ClassWithInt>(R"({"count": 7})");

    check(val.count == 7);
};

auto loadDouble = test("Load double") = []
{
    auto val = createFromJSONString<ClassWithDouble>(R"({"ratio": 2.72})");

    check(val.ratio == 2.72);
};

auto loadString = test("Load string") = []
{
    auto val = createFromJSONString<ClassWithString>(R"({"name": "world"})");

    check(val.name == "world");
};

// --- Object tests ---

auto saveObject = test("Save object") = []
{
    auto val = Inner {5};
    auto json = toJSON(val);

    check(json["x"].isNumber());
    check(json["x"].asNumber() == 5.0);
};

auto loadObject = test("Load object") = []
{
    auto val = createFromJSONString<Inner>(R"({"x": 99})");

    check(val.x == 99);
};

// --- Nested object tests ---

auto saveNested = test("Save nested objects") = []
{
    auto val = Outer {10, {20}, "test"};
    auto json = toJSON(val);

    check(json["a"].asNumber() == 10.0);
    check(json["nested"]["x"].asNumber() == 20.0);
    check(json["label"].asString() == "test");
};

auto loadNested = test("Load nested objects") = []
{
    auto val = createFromJSONString<Outer>(
        R"({"a": 77, "nested": {"x": 88}, "label": "loaded"})");

    check(val.a == 77);
    check(val.nested.x == 88);
    check(val.label == "loaded");
};

// --- Vector tests ---

auto saveVectorOfInts = test("Save vector of ints") = []
{
    auto val = ClassWithVectorOfInts {};
    auto json = toJSON(val);

    check(json["nums"].isArray());
    auto& arr = json["nums"].asArray();
    check(arr.size() == 3);
    check(arr[0].asNumber() == 1.0);
    check(arr[1].asNumber() == 2.0);
    check(arr[2].asNumber() == 3.0);
};

auto loadVectorOfInts = test("Load vector of ints") = []
{
    auto val = createFromJSONString<ClassWithVectorOfInts>(
        R"({"nums": [10, 20, 30]})");

    check(val.nums.size() == 3);
    check(val.nums[0] == 10);
    check(val.nums[1] == 20);
    check(val.nums[2] == 30);
};

auto saveVectorOfObjects = test("Save vector of objects") = []
{
    auto val = ClassWithVectorOfObjects {};
    auto json = toJSON(val);

    check(json["items"].isArray());
    auto& arr = json["items"].asArray();
    check(arr.size() == 3);
    check(arr[0]["x"].asNumber() == 1.0);
    check(arr[1]["x"].asNumber() == 2.0);
    check(arr[2]["x"].asNumber() == 3.0);
};

auto loadVectorOfObjects = test("Load vector of objects") = []
{
    auto val = createFromJSONString<ClassWithVectorOfObjects>(
        R"({"items": [{"x": 5}, {"x": 10}]})");

    check(val.items.size() == 2);
    check(val.items[0].x == 5);
    check(val.items[1].x == 10);
};

auto vectorRoundtrip = test("Vector roundtrip") = []
{
    auto original = ClassWithVectorOfStrings {};
    auto json = toJSON(original);
    auto loaded = createFromJSON<ClassWithVectorOfStrings>(json);

    check(loaded.tags.size() == 3);
    check(loaded.tags[0] == "a");
    check(loaded.tags[1] == "b");
    check(loaded.tags[2] == "c");
};

// --- std::array tests ---

auto saveArrayOfDoubles = test("Save array of doubles") = []
{
    auto val = ClassWithArrayOfDoubles {};
    auto json = toJSON(val);

    check(json["vals"].isArray());
    auto& arr = json["vals"].asArray();
    check(arr.size() == 3);
    check(arr[0].asNumber() == 1.0);
    check(arr[1].asNumber() == 2.0);
    check(arr[2].asNumber() == 3.0);
};

auto loadArrayOfDoubles = test("Load array of doubles") = []
{
    auto val = createFromJSONString<ClassWithArrayOfDoubles>(
        R"({"vals": [4.0, 5.0, 6.0]})");

    check(val.vals[0] == 4.0);
    check(val.vals[1] == 5.0);
    check(val.vals[2] == 6.0);
};

auto saveArrayOfObjects = test("Save array of objects") = []
{
    auto val = ClassWithArrayOfObjects {};
    auto json = toJSON(val);

    check(json["items"].isArray());
    auto& arr = json["items"].asArray();
    check(arr.size() == 2);
    check(arr[0]["x"].asNumber() == 7.0);
    check(arr[1]["x"].asNumber() == 8.0);
};

auto loadArrayOfObjects = test("Load array of objects") = []
{
    auto val = createFromJSONString<ClassWithArrayOfObjects>(
        R"({"items": [{"x": 11}, {"x": 22}]})");

    check(val.items[0].x == 11);
    check(val.items[1].x == 22);
};

// --- Roundtrip test ---

auto roundtrip = test("Save then load roundtrip") = []
{
    auto original = Outer {10, {20}, "hello"};
    auto json = toJSON(original);
    auto loaded = createFromJSON<Outer>(json);

    check(loaded.a == original.a);
    check(loaded.nested.x == original.nested.x);
    check(loaded.label == original.label);
};

// --- Map tests ---

auto saveMapOfStrings = test("Save map of strings") = []
{
    auto val = ClassWithStringMap {};
    auto json = toJSON(val);

    check(json["data"].isObject());
    check(json["data"]["a"].asString() == "hello");
    check(json["data"]["b"].asString() == "world");
};

auto loadMapOfStrings = test("Load map of strings") = []
{
    auto val = createFromJSONString<ClassWithStringMap>(
        R"({"data": {"x": "one", "y": "two"}})");

    check(val.data.size() == 2);
    check(val.data["x"] == "one");
    check(val.data["y"] == "two");
};

auto saveMapOfInts = test("Save map of ints") = []
{
    auto val = ClassWithIntMap {};
    auto json = toJSON(val);

    check(json["counts"]["apples"].asNumber() == 3.0);
    check(json["counts"]["bananas"].asNumber() == 5.0);
};

auto loadMapOfObjects = test("Load map of objects") = []
{
    auto val = createFromJSONString<ClassWithObjectMap>(
        R"({"items": {"first": {"x": 1}, "second": {"x": 2}}})");

    check(val.items.size() == 2);
    check(val.items["first"].x == 1);
    check(val.items["second"].x == 2);
};

auto mapRoundtrip = test("Map roundtrip") = []
{
    auto original = ClassWithIntMap {};
    auto json = toJSON(original);
    auto loaded = createFromJSON<ClassWithIntMap>(json);

    check(loaded.counts.size() == 2);
    check(loaded.counts["apples"] == 3);
    check(loaded.counts["bananas"] == 5);
};

// --- fromJSON test ---

auto fromJSONTest = test("fromJSON into existing object") = []
{
    auto val = Inner {99};
    fromJSONString(val, R"({"x": 42})");

    check(val.x == 42);
};

// --- JSON string tests ---

auto toJSONStringTest = test("toJSONString") = []
{
    auto val = Inner {5};
    auto loaded = createFromJSONString<Inner>(toJSONString(val));

    check(loaded.x == 5);
};

auto fromJSONStringTest = test("fromJSONString") = []
{
    auto val = Inner {};
    fromJSONString(val, R"({"x": 77})");

    check(val.x == 77);
};

auto createFromJSONStringTest = test("createFromJSONString") = []
{
    auto val = createFromJSONString<Outer>(
        R"({"a": 1, "nested": {"x": 2}, "label": "hi"})");

    check(val.a == 1);
    check(val.nested.x == 2);
    check(val.label == "hi");
};

// --- Missing property tests ---

auto missingPropertyKeepsDefault = test("Missing property keeps existing value") = []
{
    auto val = Outer {10, {20}, "original"};
    fromJSONString(val, R"({"a": 99})");

    check(val.a == 99);
    check(val.nested.x == 20);
    check(val.label == "original");
};

// --- MIRO_REFLECT macro tests ---

auto saveMacroReflected = test("Save MIRO_REFLECT struct") = []
{
    auto val = MacroReflected {};
    auto json = toJSON(val);

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
    auto val = createFromJSONString<MacroReflected>(
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
    auto loaded = createFromJSON<MacroReflected>(toJSON(original));

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
    auto json = toJSON(val);

    check(json.isObject());
    check(json.asObject().empty());
};

// --- MIRO_REFLECT_EXTERNAL macro tests ---

auto saveExternalReflected = test("Save MIRO_REFLECT_EXTERNAL struct") = []
{
    auto val = ExternalPoint {};
    auto json = toJSON(val);

    check(json["x"].asNumber() == 3.0);
    check(json["y"].asNumber() == 4.0);
};

auto loadExternalReflected = test("Load MIRO_REFLECT_EXTERNAL struct") = []
{
    auto val = createFromJSONString<ExternalPoint>(R"({"x": 50, "y": 60})");

    check(val.x == 50);
    check(val.y == 60);
};

auto saveExternalNested =
    test("Save MIRO_REFLECT_EXTERNAL with nested external") = []
{
    auto val = ExternalPerson {"Ada", 36, {7, 8}};
    auto json = toJSON(val);

    check(json["name"].asString() == "Ada");
    check(json["age"].asNumber() == 36.0);
    check(json["location"]["x"].asNumber() == 7.0);
    check(json["location"]["y"].asNumber() == 8.0);
};

auto loadExternalNested =
    test("Load MIRO_REFLECT_EXTERNAL with nested external") = []
{
    auto val = createFromJSONString<ExternalPerson>(
        R"({"name": "Grace", "age": 85, "location": {"x": 11, "y": 22}})");

    check(val.name == "Grace");
    check(val.age == 85);
    check(val.location.x == 11);
    check(val.location.y == 22);
};

auto externalReflectedRoundtrip = test("MIRO_REFLECT_EXTERNAL roundtrip") = []
{
    auto original = ExternalPerson {"rt", 99, {-1, -2}};
    auto loaded = createFromJSON<ExternalPerson>(toJSON(original));

    check(loaded.name == original.name);
    check(loaded.age == original.age);
    check(loaded.location.x == original.location.x);
    check(loaded.location.y == original.location.y);
};

auto externalEmpty = test("MIRO_REFLECT_EXTERNAL with no fields") = []
{
    auto val = ExternalEmpty {};
    auto json = toJSON(val);

    check(json.isObject());
    check(json.asObject().empty());
};

auto externalWithContainers =
    test("MIRO_REFLECT_EXTERNAL composes with containers") = []
{
    auto original = ExternalWithContainers {};
    auto json = toJSON(original);

    check(json["ids"].isArray());
    check(json["ids"].asArray().size() == 3);
    check(json["points"]["a"]["x"].asNumber() == 1.0);
    check(json["points"]["b"]["y"].asNumber() == 4.0);

    auto loaded = createFromJSON<ExternalWithContainers>(json);
    check(loaded.ids == original.ids);
    check(loaded.points["a"].x == 1);
    check(loaded.points["b"].y == 4);
};

auto externalVectorOfExternal =
    test("std::vector of MIRO_REFLECT_EXTERNAL type") = []
{
    auto original = std::vector<ExternalPoint> {{1, 2}, {3, 4}, {5, 6}};
    auto json = toJSON(original);

    check(json.isArray());
    check(json.asArray().size() == 3);
    check(json[2]["x"].asNumber() == 5.0);

    auto loaded = createFromJSON<std::vector<ExternalPoint>>(json);
    check(loaded.size() == 3);
    check(loaded[1].x == 3);
    check(loaded[1].y == 4);
};

// --- MIRO_REFLECT_MEMBERS macro tests ---

auto saveNamedMembers = test("Save MIRO_REFLECT_MEMBERS struct") = []
{
    auto val = NamedMembers {};
    auto json = toJSON(val);

    check(json["Full Name"].asString() == "named");
    check(json["Item Count"].asNumber() == 3.0);
    check(json["price-ratio"].asNumber() == 0.5);
    check(!json.asObject().contains("name"));
    check(!json.asObject().contains("count"));
    check(!json.asObject().contains("ratio"));
};

auto loadNamedMembers = test("Load MIRO_REFLECT_MEMBERS struct") = []
{
    auto val = createFromJSONString<NamedMembers>(
        R"({"Full Name": "loaded", "Item Count": 42, "price-ratio": 2.5})");

    check(val.name == "loaded");
    check(val.count == 42);
    check(val.ratio == 2.5);
};

auto namedMembersRoundtrip = test("MIRO_REFLECT_MEMBERS roundtrip") = []
{
    auto original = NamedMembers {"rt", 11, 0.125};
    auto loaded = createFromJSON<NamedMembers>(toJSON(original));

    check(loaded.name == original.name);
    check(loaded.count == original.count);
    check(loaded.ratio == original.ratio);
};

auto namedMembersIgnoresBareName =
    test("MIRO_REFLECT_MEMBERS ignores bare field name") = []
{
    auto val = createFromJSONString<NamedMembers>(
        R"({"name": "wrong", "Full Name": "right"})");

    check(val.name == "right");
};

auto namedMembersEmpty = test("MIRO_REFLECT_MEMBERS with no pairs") = []
{
    auto val = NamedMembersEmpty {};
    auto json = toJSON(val);

    check(json.isObject());
    check(json.asObject().empty());
};

// --- MIRO_REFLECT_EXTERNAL_MEMBERS macro tests ---

auto saveExternalNamed = test("Save MIRO_REFLECT_EXTERNAL_MEMBERS struct") = []
{
    auto val = ExternalNamed {};
    auto json = toJSON(val);

    check(json["Unit Price"].asNumber() == 9.99);
    check(json["In Stock"].asBool() == true);
    check(!json.asObject().contains("price"));
    check(!json.asObject().contains("inStock"));
};

auto loadExternalNamed = test("Load MIRO_REFLECT_EXTERNAL_MEMBERS struct") = []
{
    auto val = createFromJSONString<ExternalNamed>(
        R"({"Unit Price": 12.5, "In Stock": false})");

    check(val.price == 12.5);
    check(val.inStock == false);
};

auto externalNamedRoundtrip = test("MIRO_REFLECT_EXTERNAL_MEMBERS roundtrip") = []
{
    auto original = ExternalNamed {2.25, false};
    auto loaded = createFromJSON<ExternalNamed>(toJSON(original));

    check(loaded.price == original.price);
    check(loaded.inStock == original.inStock);
};

auto externalNamedInVector =
    test("std::vector of MIRO_REFLECT_EXTERNAL_MEMBERS type") = []
{
    auto original = std::vector<ExternalNamed> {{1.0, true}, {2.0, false}};
    auto json = toJSON(original);

    check(json.isArray());
    check(json[0]["Unit Price"].asNumber() == 1.0);
    check(json[1]["In Stock"].asBool() == false);

    auto loaded = createFromJSON<std::vector<ExternalNamed>>(json);
    check(loaded.size() == 2);
    check(loaded[0].price == 1.0);
    check(loaded[1].inStock == false);
};

// --- toJSON with const T& ---

auto toJSONAcceptsConstRef = test("toJSON accepts const T&") = []
{
    const auto val = Inner {5};
    auto json = toJSON(val);

    check(json["x"].asNumber() == 5.0);
};

auto toJSONAcceptsRvalue = test("toJSON accepts rvalue") = []
{
    auto json = toJSON(Inner {9});

    check(json["x"].asNumber() == 9.0);
};

auto toJSONStringAcceptsConstRef = test("toJSONString accepts const T&") = []
{
    const auto val = Inner {3};
    auto loaded = createFromJSONString<Inner>(toJSONString(val));

    check(loaded.x == 3);
};

// --- Json::Any ---

auto anyDefaultConstructs = test("Any default constructs to null") = []
{
    auto any = Json::Any {};

    check(any.isNull());
};

auto anyFromBool = test("Any from bool uses inherited ctor") = []
{
    auto any = Json::Any {true};

    check(any.isBool());
    check(any.asBool() == true);
};

auto anyFromInt = test("Any from int uses inherited ctor") = []
{
    auto any = Json::Any {42};

    check(any.isNumber());
    check(any.asNumber() == 42.0);
};

auto anyFromDouble = test("Any from double uses inherited ctor") = []
{
    auto any = Json::Any {3.14};

    check(any.isNumber());
    check(any.asNumber() == 3.14);
};

auto anyFromString = test("Any from string uses inherited ctor") = []
{
    auto any = Json::Any {std::string {"hi"}};

    check(any.isString());
    check(any.asString() == "hi");
};

auto anyFromCString = test("Any from C string literal") = []
{
    auto any = Json::Any {"hello"};

    check(any.isString());
    check(any.asString() == "hello");
};

auto anyFromArray = test("Any from Json::Array") = []
{
    auto any = Json::Any {Json::Array {1.0, 2.0, 3.0}};

    check(any.isArray());
    check(any.asArray().size() == 3);
};

auto anyFromObject = test("Any from Json::Object") = []
{
    auto obj = Json::Object {};
    obj["k"] = 7;
    auto any = Json::Any {obj};

    check(any.isObject());
    check(any["k"].asNumber() == 7.0);
};

auto anyFromReflectable = test("Any from reflectable struct") = []
{
    auto any = Json::Any {Inner {5}};

    check(any.isObject());
    check(any["x"].asNumber() == 5.0);
};

auto anyFromNestedReflectable = test("Any from nested reflectable struct") = []
{
    auto any = Json::Any {Outer {1, Inner {2}, "three"}};

    check(any["a"].asNumber() == 1.0);
    check(any["nested"]["x"].asNumber() == 2.0);
    check(any["label"].asString() == "three");
};

auto anyFromMacroReflectable = test("Any from MIRO_REFLECT struct") = []
{
    auto any = Json::Any {MacroInner {17}};

    check(any["x"].asNumber() == 17.0);
};

auto anyFromVectorReflectable = test("Any from std::vector") = []
{
    auto any = Json::Any {std::vector<int> {10, 20, 30}};

    check(any.isArray());
    check(any.asArray().size() == 3);
    check(any[0].asNumber() == 10.0);
    check(any[2].asNumber() == 30.0);
};

auto anyFromMapReflectable = test("Any from std::map") = []
{
    auto any = Json::Any {std::map<std::string, int> {{"a", 1}, {"b", 2}}};

    check(any.isObject());
    check(any["a"].asNumber() == 1.0);
    check(any["b"].asNumber() == 2.0);
};

auto anyFromValue = test("Any from JSON copies") = []
{
    auto value = JSON {42};
    auto any = Json::Any {value};

    check(any.asNumber() == 42.0);
};

auto anyUsableAsJsonValue = test("Any is usable where JSON is expected") = []
{
    auto any = Json::Any {Inner {8}};
    const JSON& asBase = any;

    check(asBase["x"].asNumber() == 8.0);
};

auto anyAsReturnType = test("Any as return type converts implicitly") = []
{
    auto makeJson = [](int tagToUse) -> Json::Any
    {
        if (tagToUse == 0)
            return Inner {1};
        return MacroReflected {};
    };

    auto first = makeJson(0);
    check(first["x"].asNumber() == 1.0);

    auto second = makeJson(1);
    check(second["name"].asString() == "macro");
};

auto anyPrintsRoundTrip = test("Any round-trips through print/parse") = []
{
    auto any = Json::Any {Outer {11, Inner {22}, "rt"}};
    auto printed = Json::print(any);
    auto parsed = Json::parse(printed);

    check(parsed["a"].asNumber() == 11.0);
    check(parsed["nested"]["x"].asNumber() == 22.0);
    check(parsed["label"].asString() == "rt");
};

auto anyCopyConstructs = test("Any copy constructs from Any") = []
{
    auto original = Json::Any {Inner {4}};
    auto copy = original;

    check(copy["x"].asNumber() == 4.0);
    check(original["x"].asNumber() == 4.0);
};

// --- Integral overloads (non-int, non-bool) ---

auto saveIntegrals = test("Save non-int integral types as numbers") = []
{
    auto val = ClassWithIntegrals {};
    auto json = toJSON(val);

    check(json["u"].isNumber());
    check(json["u"].asNumber() == 5.0);
    check(json["s"].asNumber() == -3.0);
    check(json["ll"].asNumber() == 1234567890123.0);
    check(json["c"].asNumber() == double('A'));
};

auto loadIntegrals = test("Load non-int integral types from numbers") = []
{
    auto val = createFromJSONString<ClassWithIntegrals>(
        R"({"u": 17, "s": -42, "ll": 9999999999, "c": 66})");

    check(val.u == 17u);
    check(val.s == -42);
    check(val.ll == 9999999999LL);
    check(val.c == 'B');
};

auto integralsRoundtrip = test("Integral types roundtrip") = []
{
    auto original = ClassWithIntegrals {123u, -7, 5000000000LL, 'Z'};
    auto loaded = createFromJSON<ClassWithIntegrals>(toJSON(original));

    check(loaded.u == original.u);
    check(loaded.s == original.s);
    check(loaded.ll == original.ll);
    check(loaded.c == original.c);
};

// --- std::optional ---

auto saveOptionalEmpty = test("Save empty std::optional as null") = []
{
    auto val = ClassWithOptional {};
    auto json = toJSON(val);

    check(json["maybeInt"].isNull());
    check(json["maybeInner"].isNull());
};

auto saveOptionalEngaged = test("Save engaged std::optional inlines value") = []
{
    auto val = ClassWithOptional {};
    val.maybeInt = 42;
    val.maybeInner = Inner {7};
    auto json = toJSON(val);

    check(json["maybeInt"].asNumber() == 42.0);
    check(json["maybeInner"]["x"].asNumber() == 7.0);
};

auto loadOptionalNull = test("Load null into std::optional clears it") = []
{
    auto val = ClassWithOptional {};
    val.maybeInt = 99;
    fromJSONString(val, R"({"maybeInt": null, "maybeInner": null})");

    check(!val.maybeInt.has_value());
    check(!val.maybeInner.has_value());
};

auto loadOptionalValue = test("Load value into std::optional") = []
{
    auto val = createFromJSONString<ClassWithOptional>(
        R"({"maybeInt": 11, "maybeInner": {"x": 22}})");

    check(val.maybeInt.has_value());
    check(*val.maybeInt == 11);
    check(val.maybeInner.has_value());
    check(val.maybeInner->x == 22);
};

auto loadOptionalMissingKeyKeepsValue =
    test("Missing optional key keeps existing value") = []
{
    auto val = ClassWithOptional {};
    val.maybeInt = 7;
    fromJSONString(val, R"({})");

    check(val.maybeInt.has_value());
    check(*val.maybeInt == 7);
};

auto optionalRoundtrip = test("std::optional roundtrip") = []
{
    auto original = ClassWithOptional {};
    original.maybeInt = 3;
    auto loaded = createFromJSON<ClassWithOptional>(toJSON(original));

    check(loaded.maybeInt.has_value());
    check(*loaded.maybeInt == 3);
    check(!loaded.maybeInner.has_value());
};

// --- Enum reflection ---

auto enumToStringWorks = test("enumToString returns enumerator name") = []
{
    check(enumToString(Color::Red) == "Red");
    check(enumToString(Color::Green) == "Green");
    check(enumToString(Color::Blue) == "Blue");
};

auto enumToStringUnknown = test("enumToString returns empty for unknown") = []
{
    auto unknown = static_cast<Color>(99);
    check(enumToString(unknown).empty());
};

auto enumFromStringWorks = test("enumFromString parses enumerator name") = []
{
    auto parsed = enumFromString<Color>("Green");
    check(parsed.has_value());
    check(*parsed == Color::Green);
};

auto enumFromStringUnknown = test("enumFromString returns nullopt for unknown") = []
{
    auto parsed = enumFromString<Color>("Purple");
    check(!parsed.has_value());
};

auto enumFromStringUnscoped = test("enumFromString works for unscoped enums") = []
{
    auto parsed = enumFromString<UnscopedMode>("ModeOn");
    check(parsed.has_value());
    check(*parsed == ModeOn);
};

auto saveEnumAsString = test("Save enum as JSON string") = []
{
    auto val = ClassWithEnum {};
    auto json = toJSON(val);

    check(json["color"].isString());
    check(json["color"].asString() == "Green");
    check(json["signal"].asString() == "Go");
    check(json["mode"].asString() == "ModeAuto");
};

auto loadEnumFromString = test("Load enum from JSON string") = []
{
    auto val = createFromJSONString<ClassWithEnum>(
        R"({"color": "Blue", "signal": "Stop", "mode": "ModeOn"})");

    check(val.color == Color::Blue);
    check(val.signal == Signal::Stop);
    check(val.mode == ModeOn);
};

auto enumRoundtrip = test("Enum roundtrip") = []
{
    auto original = ClassWithEnum {Color::Red, Signal::Wait, ModeOff};
    auto loaded = createFromJSON<ClassWithEnum>(toJSON(original));

    check(loaded.color == original.color);
    check(loaded.signal == original.signal);
    check(loaded.mode == original.mode);
};

auto loadEnumFromNumber = test("Load enum from JSON number") = []
{
    auto val = createFromJSONString<ClassWithEnum>(
        R"({"color": 2, "signal": -1, "mode": 1})");

    check(val.color == Color::Blue);
    check(val.signal == Signal::Stop);
    check(val.mode == ModeOn);
};

auto loadUnknownEnumKeepsValue =
    test("Load unknown enum string keeps existing value") = []
{
    auto val = ClassWithEnum {};
    fromJSONString(val, R"({"color": "Magenta"})");

    check(val.color == Color::Green);
};

auto saveOutOfRangeEnumAsNumber =
    test("Save out-of-range enum value falls back to number") = []
{
    auto val = ClassWithEnum {};
    val.color = static_cast<Color>(99);
    auto json = toJSON(val);

    check(json["color"].isNumber());
    check(json["color"].asNumber() == 99.0);
};

auto outOfRangeEnumRoundtrip =
    test("Out-of-range enum roundtrips through number") = []
{
    auto original = ClassWithEnum {};
    original.color = static_cast<Color>(77);
    auto loaded = createFromJSON<ClassWithEnum>(toJSON(original));

    check(static_cast<int>(loaded.color) == 77);
};
