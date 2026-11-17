// PrimitiveRef::storage: where a primitive really lives. A float or a
// narrow integer is visited through a widened double / int64 copy in
// `data`, while `storage` / `storageType` point at the field itself and
// name its C++ type. Reflectors that only use `data` see what they always
// did; a loading reflector may write either, and the field keeps the write.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

struct Mixed
{
    MIRO_REFLECT(flag, count, ratio, label, wide, small, tiny, gain, big, lng)

    bool flag = true;
    int count = 3;
    double ratio = 0.5;
    std::string label = "a";
    std::int64_t wide = 7;
    std::uint16_t small = 600;
    std::int8_t tiny = -5;
    float gain = 1.25f;
    std::uint64_t big = 9;
    long lng = 11;
};

struct SeenPrimitive
{
    const void* data = nullptr;
    const void* storage = nullptr;
    StorageType storageType = StorageType::Bool;
    bool narrowed = false;
};

const void* dataAddress(const PrimitiveRef& ref)
{
    return std::visit([](auto* value) -> const void* { return value; }, ref.data);
}

// Records every primitive; on a load, writes `loadedGain` straight into
// a float field and leaves the widened copy alone.
struct StorageRecorder : Reflector
{
    StorageRecorder(std::vector<SeenPrimitive>& seenToUse, Options optsToUse)
        : Reflector(optsToUse)
        , seen(seenToUse)
    {
    }

    void visit(PrimitiveRef ref) override
    {
        seen.push_back(
            {dataAddress(ref), ref.storage, ref.storageType, ref.isNarrowed()});

        if (isLoading())
            if (auto* gain = ref.storageAs<float>())
                *gain = loadedGain;
    }

    void writeNull() override {}
    ValueKind kind() const override { return ValueKind::Absent; }

    Reflector& atKey(std::string_view, Options childOpts) override
    {
        child = std::make_unique<StorageRecorder>(seen, childOpts);
        return *child;
    }

    Reflector& atIndex(std::size_t, Options childOpts) override
    {
        child = std::make_unique<StorageRecorder>(seen, childOpts);
        return *child;
    }

    static constexpr auto loadedGain = 4.5f;

    std::vector<SeenPrimitive>& seen;
    std::unique_ptr<StorageRecorder> child;
};

std::vector<SeenPrimitive> walk(Mixed& value, Mode mode)
{
    auto seen = std::vector<SeenPrimitive> {};
    auto root = StorageRecorder {seen, Detail::topLevelOptions<Mixed>(mode)};
    Detail::reflectValue(root, value);
    return seen;
}

} // namespace

static_assert(storageTypeOf<int>() == StorageType::Int32);
static_assert(storageTypeOf<std::int32_t>() == StorageType::Int32);
static_assert(storageTypeOf<std::uint8_t>() == StorageType::UInt8);
static_assert(storageTypeOf<std::int64_t>() == StorageType::Int64);
static_assert(storageTypeOf<float>() == StorageType::Float);
static_assert(storageTypeOf<std::string>() == StorageType::String);

auto directPrimitivesStoreInPlace =
    test("PrimitiveStorage: wide primitives are stored where data points") = []
{
    auto value = Mixed {};
    auto seen = walk(value, Mode::Save);

    check(seen.size() == 10);

    auto expected = std::vector<std::pair<const void*, StorageType>> {
        {&value.flag, StorageType::Bool},
        {&value.count, StorageType::Int32},
        {&value.ratio, StorageType::Double},
        {&value.label, StorageType::String},
        {&value.wide, StorageType::Int64},
    };

    for (auto index = std::size_t {0}; index < expected.size(); ++index)
    {
        check(seen[index].storage == expected[index].first);
        check(seen[index].data == expected[index].first);
        check(seen[index].storageType == expected[index].second);
        check(!seen[index].narrowed);
    }
};

auto narrowedPrimitivesExposeField =
    test("PrimitiveStorage: narrowed primitives expose the real field") = []
{
    auto value = Mixed {};
    auto seen = walk(value, Mode::Save);

    check(seen.size() == 10);

    auto expected = std::vector<std::pair<const void*, StorageType>> {
        {&value.small, StorageType::UInt16},
        {&value.tiny, StorageType::Int8},
        {&value.gain, StorageType::Float},
        {&value.big, StorageType::UInt64},
        {&value.lng, storageTypeOf<long>()},
    };

    for (auto index = std::size_t {0}; index < expected.size(); ++index)
    {
        auto& primitive = seen[index + 5];
        check(primitive.storage == expected[index].first);
        check(primitive.data != expected[index].first);
        check(primitive.storageType == expected[index].second);
        check(primitive.narrowed);
    }
};

auto storageAsMatchesType = test("PrimitiveStorage: storageAs checks the type") = []
{
    auto gain = 2.f;
    auto wide = static_cast<double>(gain);
    auto ref = PrimitiveRef::narrowed(wide, gain);

    check(ref.storageAs<float>() == &gain);
    check(ref.storageAs<double>() == nullptr);
    check(ref.isNarrowed());

    auto count = 1;
    auto direct = PrimitiveRef {count};
    check(direct.storageAs<int>() == &count);
    check(direct.storageAs<std::int32_t>() == &count);
    check(direct.storageAs<std::uint32_t>() == nullptr);
    check(!direct.isNarrowed());
};

auto loadThroughStorageSticks =
    test("PrimitiveStorage: a load written through storage is kept") = []
{
    auto value = Mixed {};
    walk(value, Mode::Load);

    check(value.gain == StorageRecorder::loadedGain);
    check(value.tiny == -5);
    check(value.small == 600);
};

auto narrowedJsonRoundTrip =
    test("PrimitiveStorage: narrowed fields still round-trip through JSON") = []
{
    auto value = Mixed {};
    value.small = 65535;
    value.tiny = -128;
    value.gain = -0.375f;
    value.big = 1234567;
    value.lng = -42;

    auto json = toJSONString(value);
    auto loaded = createFromJSONString<Mixed>(json);

    check(loaded.small == 65535);
    check(loaded.tiny == -128);
    check(loaded.gain == -0.375f);
    check(loaded.big == 1234567);
    check(loaded.lng == -42);
    check(toJSONString(loaded) == json);

    auto changed = createFromJSONString<Mixed>(
        R"({"small":7,"tiny":3,"gain":0.5,"big":2,"lng":5})");
    check(changed.small == 7);
    check(changed.tiny == 3);
    check(changed.gain == 0.5f);
    check(changed.big == 2);
    check(changed.lng == 5);
};
