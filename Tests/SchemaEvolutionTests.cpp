// Schema evolution: data written by one version of a struct, read by the
// next, through JSON.
//
// The rules these tests pin:
// - Fields are matched by name, never by position: reordering is free,
//   an unknown field is skipped, a missing field keeps the value the
//   reader started with (its default member initializer).
// - A renamed field is a missing field plus an unknown one.
// - A value whose type no longer fits the field is refused and the field
//   keeps its prior value: no wraparound, no out-of-range casts.
// - Truncated or corrupted bytes never crash or read out of bounds.

#include <Miro/Reflect.h>
#include <Miro/Xml.h>

#include <NanoTest/NanoTest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

template <typename To, typename From>
To convertThroughJson(const From& value)
{
    return createFromJSONString<To>(toJSONString(value));
}

// Writes `value` as From, reads it back as To, and hands the result to
// `expect`.
template <typename To, typename From, typename Expect>
void checkThroughJson(const From& value, Expect expect)
{
    expect(convertThroughJson<To>(value));
}

// --- The running example: a field renamed between versions ---

struct ProfileV1
{
    std::uint32_t field1 = 0;
    std::string field2;

    MIRO_REFLECT(field1, field2)
};

// Renamed with no migration: field2's data is simply orphaned.
struct ProfileV2Unmigrated
{
    std::uint32_t field1 = 0;
    std::string fieldTwo = "unset";

    MIRO_REFLECT(field1, fieldTwo)
};

ProfileV1 sampleV1()
{
    return {.field1 = 4000000000u, .field2 = "hello"};
}

auto renameUnmigrated =
    test("Evolution: renamed field without migration keeps its default") = []
{
    checkThroughJson<ProfileV2Unmigrated>(sampleV1(),
                                          [](const ProfileV2Unmigrated& v2)
                                          {
                                              check(v2.field1 == 4000000000u);
                                              check(v2.fieldTwo == "unset");
                                          });
};

// --- Added, removed, reordered fields ---

struct AddedV2
{
    std::uint32_t field1 = 0;
    std::string field2;
    int added = 42;
    std::vector<int> addedList {1, 2};
    std::map<std::string, int> addedMap {{"k", 1}};
    std::optional<int> addedOptional;
    std::optional<int> addedOptionalWithValue = 5;

    MIRO_REFLECT(field1,
                 field2,
                 added,
                 addedList,
                 addedMap,
                 addedOptional,
                 addedOptionalWithValue)
};

auto fieldAdded = test("Evolution: fields added in V2 keep their defaults") = []
{
    checkThroughJson<AddedV2>(sampleV1(),
                              [](const AddedV2& v2)
                              {
                                  check(v2.field1 == 4000000000u);
                                  check(v2.field2 == "hello");
                                  check(v2.added == 42);
                                  check(v2.addedList == std::vector<int> {1, 2});
                                  check(v2.addedMap.size() == 1);
                                  check(v2.addedMap.at("k") == 1);
                                  check(!v2.addedOptional.has_value());
                                  check(v2.addedOptionalWithValue == 5);
                              });
};

struct WideV1
{
    std::uint32_t field1 = 0;
    std::string field2;
    std::vector<double> dropped;
    std::optional<std::string> droppedOptional;
    std::map<std::string, ProfileV1> droppedMap;
    int tail = 0;

    MIRO_REFLECT(field1, field2, dropped, droppedOptional, droppedMap, tail)
};

struct NarrowV2
{
    std::uint32_t field1 = 0;
    std::string field2;
    int tail = 0;

    MIRO_REFLECT(field1, field2, tail)
};

auto fieldRemoved = test("Evolution: fields removed in V2 are skipped") = []
{
    auto v1 = WideV1 {};
    v1.field1 = 7;
    v1.field2 = "s";
    v1.dropped = {0.1, 2.0, 1e300};
    v1.droppedOptional = "gone";
    v1.droppedMap = {{"a", sampleV1()}};
    v1.tail = -3;

    checkThroughJson<NarrowV2>(v1,
                               [](const NarrowV2& v2)
                               {
                                   check(v2.field1 == 7);
                                   check(v2.field2 == "s");
                                   check(v2.tail == -3);
                               });
};

struct ReorderedV2
{
    int tail = 0;
    std::string field2;
    std::uint32_t field1 = 0;

    MIRO_REFLECT(tail, field2, field1)
};

auto fieldsReordered = test("Evolution: reordered fields load by name") = []
{
    auto v1 = NarrowV2 {.field1 = 11, .field2 = "x", .tail = 12};

    checkThroughJson<ReorderedV2>(v1,
                                  [](const ReorderedV2& v2)
                                  {
                                      check(v2.field1 == 11);
                                      check(v2.field2 == "x");
                                      check(v2.tail == 12);
                                  });
};

struct OptionalV1
{
    std::optional<int> maybe = 9;
    std::optional<int> nulled;
    int kept = 1;

    MIRO_REFLECT(maybe, nulled, kept)
};

struct OptionalRemovedV2
{
    int kept = 0;
    MIRO_REFLECT(kept)
};

struct OptionalToPlainV2
{
    int maybe = -1;
    int nulled = -1;
    int kept = 0;

    MIRO_REFLECT(maybe, nulled, kept)
};

struct PlainToOptionalV2
{
    std::optional<int> kept;
    MIRO_REFLECT(kept)
};

auto optionalRemoved =
    test("Evolution: optional fields removed in V2 are skipped") = []
{
    checkThroughJson<OptionalRemovedV2>(
        OptionalV1 {}, [](const OptionalRemovedV2& v2) { check(v2.kept == 1); });
};

auto optionalUnwrapped =
    test("Evolution: optional<int> -> int takes the value, null keeps default") = []
{
    checkThroughJson<OptionalToPlainV2>(OptionalV1 {},
                                        [](const OptionalToPlainV2& v2)
                                        {
                                            check(v2.maybe == 9);
                                            check(v2.nulled == -1);
                                            check(v2.kept == 1);
                                        });
};

auto optionalWrapped =
    test("Evolution: int -> optional<int> engages with the value") = []
{
    checkThroughJson<PlainToOptionalV2>(
        OptionalV1 {}, [](const PlainToOptionalV2& v2) { check(v2.kept == 1); });
};

// --- Evolved types inside containers ---

struct OuterV1
{
    ProfileV1 profile;
    std::vector<ProfileV1> list;
    std::map<std::string, ProfileV1> byName;

    MIRO_REFLECT(profile, list, byName)
};

struct AddedOuterV2
{
    AddedV2 profile;
    std::vector<AddedV2> list;
    std::map<std::string, AddedV2> byName;

    MIRO_REFLECT(profile, list, byName)
};

OuterV1 sampleOuterV1()
{
    auto v1 = OuterV1 {};
    v1.profile = {.field1 = 1, .field2 = "nested"};
    v1.list = {{.field1 = 2, .field2 = "first"}, {.field1 = 3, .field2 = "second"}};
    v1.byName = {{"a", {.field1 = 4, .field2 = "alpha"}},
                 {"b", {.field1 = 5, .field2 = "beta"}}};
    return v1;
}

auto nestedAdded =
    test("Evolution: nested, vector and map elements default added fields") = []
{
    checkThroughJson<AddedOuterV2>(sampleOuterV1(),
                                   [](const AddedOuterV2& v2)
                                   {
                                       check(v2.profile.added == 42);
                                       check(v2.profile.field2 == "nested");
                                       check(v2.list.size() == 2);

                                       for (const auto& element: v2.list)
                                       {
                                           check(element.added == 42);
                                           check(element.addedList.size() == 2);
                                       }

                                       check(v2.byName.at("a").added == 42);
                                       check(v2.byName.at("a").field2 == "alpha");
                                   });
};

// --- Type changes ---

template <typename T>
struct SingleFieldHolder
{
    T value {};
    int after = 0;

    MIRO_REFLECT(value, after)
};

// Writes SingleFieldHolder<From>{from}, reads SingleFieldHolder<To> starting from `prior`.
template <typename To, typename From, typename Expect>
void checkFieldTypeChange(From from, To prior, Expect expect)
{
    auto written = SingleFieldHolder<From> {from, 77};

    auto loaded = SingleFieldHolder<To> {prior, 0};
    fromJSONString(loaded, toJSONString(written));
    expect(loaded.value);
    check(loaded.after == 77);
}

auto widenUnsigned = test("Evolution: uint32 -> uint64 widens") = []
{
    checkFieldTypeChange<std::uint64_t>(std::uint32_t {4000000000u},
                                        std::uint64_t {1},
                                        [](std::uint64_t v)
                                        { check(v == 4000000000u); });
};

auto narrowUnsignedFits =
    test("Evolution: uint64 -> uint32 converts when it fits") = []
{
    checkFieldTypeChange<std::uint32_t>(std::uint64_t {123},
                                        std::uint32_t {1},
                                        [](std::uint32_t v) { check(v == 123); });
};

auto narrowUnsignedOverflow =
    test("Evolution: uint64 -> uint32 out of range keeps prior") = []
{
    checkFieldTypeChange<std::uint32_t>(std::uint64_t {1} << 40,
                                        std::uint32_t {1},
                                        [](std::uint32_t v) { check(v == 1); });
};

auto narrowSignedOverflow =
    test("Evolution: int64 -> int32 out of range keeps prior") = []
{
    checkFieldTypeChange<std::int32_t>(std::int64_t {-5000000000LL},
                                       std::int32_t {1},
                                       [](std::int32_t v) { check(v == 1); });
};

auto signedToUnsigned = test("Evolution: negative int -> unsigned keeps prior") = []
{
    checkFieldTypeChange<std::uint32_t>(
        -1, std::uint32_t {1}, [](std::uint32_t v) { check(v == 1); });
    checkFieldTypeChange<std::uint16_t>(
        -1, std::uint16_t {1}, [](std::uint16_t v) { check(v == 1); });
};

auto uint64FullRange =
    test("Evolution: uint64 past INT64_MAX still round trips") = []
{
    auto big = std::numeric_limits<std::uint64_t>::max() - 5;
    checkFieldTypeChange<std::uint64_t>(
        big, std::uint64_t {1}, [big](std::uint64_t v) { check(v == big); });
};

auto intToDouble = test("Evolution: int -> double converts") = []
{ checkFieldTypeChange<double>(-7, 0.5, [](double v) { check(v == -7.0); }); };

auto int64ToDouble = test("Evolution: int64 -> double converts") = []
{
    checkFieldTypeChange<double>(std::int64_t {1} << 52,
                                 0.5,
                                 [](double v) { check(v == 4503599627370496.0); });
};

auto doubleToFloat = test("Evolution: double -> float narrows to nearest") = []
{ checkFieldTypeChange<float>(0.1, 1.0f, [](float v) { check(v == 0.1f); }); };

auto doubleToFloatOverflow =
    test("Evolution: double -> float out of range keeps prior") = []
{ checkFieldTypeChange<float>(1e300, 1.0f, [](float v) { check(v == 1.0f); }); };

auto doubleToInt = test("Evolution: double -> int truncates toward zero") = []
{
    checkFieldTypeChange<int>(3.9, 1, [](int v) { check(v == 3); });
    checkFieldTypeChange<int>(-3.9, 1, [](int v) { check(v == -3); });
};

auto doubleToIntOverflow =
    test("Evolution: double -> int out of range keeps prior") = []
{
    checkFieldTypeChange<int>(1e300, 1, [](int v) { check(v == 1); });
    checkFieldTypeChange<int>(-1e300, 1, [](int v) { check(v == 1); });
    checkFieldTypeChange<std::int64_t>(
        1e19, std::int64_t {1}, [](std::int64_t v) { check(v == 1); });
    checkFieldTypeChange<std::uint8_t>(
        300.0, std::uint8_t {1}, [](std::uint8_t v) { check(v == 1); });
};

auto stringToInt = test("Evolution: string -> int keeps prior") = []
{ checkFieldTypeChange<int>(std::string {"12"}, 1, [](int v) { check(v == 1); }); };

auto intToString = test("Evolution: int -> string keeps prior") = []
{
    checkFieldTypeChange<std::string>(12,
                                      std::string {"prior"},
                                      [](const std::string& v)
                                      { check(v == "prior"); });
};

auto scalarToVector = test("Evolution: scalar -> vector reads as empty") = []
{
    checkFieldTypeChange<std::vector<int>>(
        5, std::vector<int> {}, [](const std::vector<int>& v) { check(v.empty()); });
};

auto vectorToScalar = test("Evolution: vector -> scalar keeps prior") = []
{
    checkFieldTypeChange<int>(
        std::vector<int> {1, 2, 3}, 1, [](int v) { check(v == 1); });
};

auto structToScalar = test("Evolution: struct -> scalar keeps prior") = []
{ checkFieldTypeChange<int>(sampleV1(), 1, [](int v) { check(v == 1); }); };

auto scalarToStruct = test("Evolution: scalar -> struct keeps prior fields") = []
{
    checkFieldTypeChange<ProfileV2Unmigrated>(
        5,
        ProfileV2Unmigrated {.field1 = 1, .fieldTwo = "prior"},
        [](const ProfileV2Unmigrated& v)
        {
            check(v.field1 == 1);
            check(v.fieldTwo == "prior");
        });
};

auto packedElementChange =
    test("Evolution: vector<double> -> vector<int> converts per element") = []
{
    // Out-of-range elements keep their prior (zero) value.
    checkFieldTypeChange<std::vector<int>>(
        std::vector<double> {1.5, -2.5, 1e300, 7.0},
        std::vector<int> {},
        [](const std::vector<int>& v)
        { check(v == std::vector<int> {1, -2, 0, 7}); });
};

auto packedNarrowing = test(
    "Evolution: vector<int64> -> vector<int32> refuses overflow per element") = []
{
    checkFieldTypeChange<std::vector<std::int32_t>>(
        std::vector<std::int64_t> {1, std::int64_t {1} << 40, -3},
        std::vector<std::int32_t> {},
        [](const std::vector<std::int32_t>& v)
        { check(v == std::vector<std::int32_t> {1, 0, -3}); });
};

// --- Damaged input for an evolved struct ---

auto truncatedJson = test("Evolution: truncated V1 JSON reads as default V2") = []
{
    auto text = toJSONString(sampleOuterV1());

    for (auto size = std::size_t {0}; size < text.size(); ++size)
    {
        auto loaded =
            createFromJSONString<AddedOuterV2>(std::string_view {text.data(), size});
        check(loaded.list.empty());
        check(loaded.profile.added == 42);
    }
};

// --- The missing-container rule outside JSON ---

auto xmlEmptyVector =
    test("Evolution: XML empty vector over a non-empty default stays empty") = []
{
    // A missing key keeps a container's prior value; an empty one that
    // was written must still read back empty.
    struct WithList
    {
        std::vector<int> items {1, 2};
        void reflect(Miro::Reflector& ref) { MIRO_FIELDS(ref, items); }
    };

    auto value = WithList {};
    value.items.clear();

    check(createFromJSONString<WithList>(toJSONString(value)).items.empty());
    check(createFromXMLString<WithList>(toXMLString(value)).items.empty());
};

} // namespace
