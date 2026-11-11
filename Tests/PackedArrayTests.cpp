#include <Miro/Reflect.h>
#include <NanoTest/NanoTest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

constexpr auto loadedArraySize = std::size_t {4};
constexpr auto firstLoadedElement = 1;

struct PackedProbeLog
{
    std::size_t packedCalls = 0;
    std::size_t packedElementCount = 0;
    std::size_t elementVisits = 0;
};

// One array slot that reports every packed and per-element visit. When
// it accepts packed arrays, a load fills them with 1, 2, 3, ...
struct PackedProbeReflector : Reflector
{
    PackedProbeReflector(PackedProbeLog& logToUse,
                         Options optsToUse,
                         bool acceptsPackedToUse)
        : Reflector(std::move(optsToUse))
        , log(logToUse)
        , acceptsPacked(acceptsPackedToUse)
    {
    }

    void visit(PrimitiveRef /*ref*/) override { ++log.elementVisits; }
    void writeNull() override {}
    ValueKind kind() const override { return ValueKind::Array; }

    bool visitPacked(PackedArrayRef ref) override
    {
        if (!acceptsPacked)
            return false;

        ++log.packedCalls;
        std::visit(
            [this](auto elements)
            {
                log.packedElementCount = elements.size();

                if (isLoading())
                    fillWithCountingNumbers(elements);
            },
            ref.data);
        return true;
    }

    std::size_t arraySize() const override { return loadedArraySize; }

    Reflector& atKey(std::string_view /*key*/, Options childOpts) override
    {
        return createChild(childOpts);
    }

    Reflector& atIndex(std::size_t /*index*/, Options childOpts) override
    {
        return createChild(childOpts);
    }

    template <typename Elements>
    static void fillWithCountingNumbers(Elements elements)
    {
        auto next = firstLoadedElement;

        for (auto& element: elements)
            element = static_cast<typename Elements::value_type>(next++);
    }

    Reflector& createChild(const Options& childOpts)
    {
        child =
            std::make_unique<PackedProbeReflector>(log, childOpts, acceptsPacked);
        return *child;
    }

    PackedProbeLog& log;
    bool acceptsPacked = true;
    std::unique_ptr<PackedProbeReflector> child;
};

template <typename T>
PackedProbeLog reflectThroughProbe(T& value, Mode mode, bool acceptsPacked)
{
    auto log = PackedProbeLog {};
    auto probe =
        PackedProbeReflector {log, Detail::topLevelOptions<T>(mode), acceptsPacked};
    Detail::reflectValue(probe, value);
    return log;
}

auto savePackedVector = test(
    "Packed arrays: a reflector that takes visitPacked gets a vector whole") = []
{
    auto values = std::vector<double> {1.5, 2.5, 3.5};
    auto log = reflectThroughProbe(values, Mode::Save, true);

    check(log.packedCalls == 1);
    check(log.packedElementCount == values.size());
    check(log.elementVisits == 0);
};

auto loadPackedVector =
    test("Packed arrays: a load resizes the vector, then visitPacked fills it") = []
{
    auto values = std::vector<std::int32_t> {};
    auto log = reflectThroughProbe(values, Mode::Load, true);

    check(log.packedCalls == 1);
    check(values == std::vector<std::int32_t> {1, 2, 3, 4});
};

auto loadPackedFixedArray =
    test("Packed arrays: a std::array loads whole through visitPacked") = []
{
    auto values = std::array<float, loadedArraySize> {};
    auto log = reflectThroughProbe(values, Mode::Load, true);

    check(log.packedCalls == 1);
    check(values == std::array<float, loadedArraySize> {1.0f, 2.0f, 3.0f, 4.0f});
};

auto declinedPackedWalksElements =
    test("Packed arrays: a reflector that declines gets one visit per element") = []
{
    auto values = std::vector<std::int64_t> {7, 8, 9};
    auto log = reflectThroughProbe(values, Mode::Save, false);

    check(log.packedCalls == 0);
    check(log.elementVisits == values.size());
};

auto unpackableElementsWalk =
    test("Packed arrays: non-packable elements never reach visitPacked") = []
{
    auto strings = std::vector<std::string> {"a", "b"};
    auto shorts = std::vector<std::int16_t> {1, 2, 3};

    auto stringLog = reflectThroughProbe(strings, Mode::Save, true);
    auto shortLog = reflectThroughProbe(shorts, Mode::Save, true);

    check(stringLog.packedCalls == 0);
    check(stringLog.elementVisits == strings.size());
    check(shortLog.packedCalls == 0);
    check(shortLog.elementVisits == shorts.size());
};

} // namespace
