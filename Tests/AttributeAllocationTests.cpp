// Field attributes must not allocate. This executable replaces the
// global operator new with a counting one (which is why it is not part
// of MiroTests) and checks that saving and loading a struct whose
// fields carry attributes performs exactly as many allocations as the
// same struct without them.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <string>

namespace
{

std::atomic<std::size_t> allocationCount {0};

} // namespace

void* operator new(std::size_t size)
{
    ++allocationCount;

    if (auto* ptr = std::malloc(size == 0 ? 1 : size))
        return ptr;

    throw std::bad_alloc {};
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

using namespace nano;
using namespace Miro;

namespace
{

struct Range
{
    float min = 0.f;
    float max = 0.f;

    MIRO_REFLECT(min, max)
};

enum class Unit
{
    hertz,
    decibel
};

struct Point
{
    double x = 1.0;
    double y = 2.0;

    MIRO_REFLECT(x, y)
};

struct PlainFields
{
    float speed = 2.5f;
    std::string city = "A city name long enough to defeat the small-string buffer";
    int count = 7;
    Point origin;

    void reflect(Reflector& ref)
    {
        ref["speed"](speed);
        ref["city"](city);
        ref["count"](count);
        ref["origin"](origin);
    }
};

struct AttributedFields
{
    float speed = 2.5f;
    std::string city = "A city name long enough to defeat the small-string buffer";
    int count = 7;
    Point origin;

    void reflect(Reflector& ref)
    {
        ref["speed"](speed, Range {0.f, 10.f}, Unit::hertz);
        ref["city"](city, Doc {"City name"});
        ref["count"].doc("How many")(count, Range {0.f, 100.f});
        ref["origin"](origin, Unit::decibel);
    }
};

template <typename T>
std::size_t allocationsDuringSave(const T& value)
{
    auto before = allocationCount.load();
    auto json = toJSON(value);
    auto after = allocationCount.load();
    return after - before;
}

template <typename T>
std::size_t allocationsDuringLoad(const JSON& json)
{
    auto value = T {};
    auto before = allocationCount.load();
    fromJSON(value, json);
    auto after = allocationCount.load();
    return after - before;
}

} // namespace

auto attributeSaveAllocations =
    test("Attribute allocations: toJSON allocates the same with attributes") = []
{
    // Warm up anything lazily initialised on first use.
    allocationsDuringSave(PlainFields {});
    allocationsDuringSave(AttributedFields {});

    auto plain = allocationsDuringSave(PlainFields {});
    auto attributed = allocationsDuringSave(AttributedFields {});

    check(plain > 0); // the counter is live
    check(plain == attributed);
};

auto attributeLoadAllocations =
    test("Attribute allocations: fromJSON allocates the same with attributes") = []
{
    auto json = toJSON(PlainFields {});

    allocationsDuringLoad<PlainFields>(json);
    allocationsDuringLoad<AttributedFields>(json);

    auto plain = allocationsDuringLoad<PlainFields>(json);
    auto attributed = allocationsDuringLoad<AttributedFields>(json);

    check(plain > 0);
    check(plain == attributed);
};
