// Compile-time coverage for the public Miro::Reflectable concept: "the
// dispatcher knows how to reflect a T". Everything here is a static_assert
// — if this TU compiles, the tests pass. The one runtime test keeps the
// file visible in the NanoTest run.

#include "TestTypes.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

struct MacroStruct
{
    int a = 0;
    std::string b;

    MIRO_REFLECT(a, b)
};

struct Plain
{
    int x = 0;
};

struct WrongReflectSignature
{
    void reflect(int) {}
};

// A const reflect() is still callable on a non-const value, so it counts.
struct ConstReflect
{
    void reflect(Miro::Reflector&) const {}
};

enum class Scoped
{
    One,
    Two
};

enum Unscoped
{
    Red,
    Green
};

struct ShapeBase
{
    virtual ~ShapeBase() = default;
    void reflect(Miro::Reflector&) {}
};

struct Circle : ShapeBase
{
    static constexpr auto miroTag = 1;
    double radius = 0.0;
    MIRO_REFLECT(radius)
};

struct Square : ShapeBase
{
    static constexpr auto miroTag = 2;
    double side = 0.0;
    MIRO_REFLECT(side)
};

struct Button
{
    static constexpr auto miroTag = 1;
    std::string label;
    MIRO_REFLECT(label)
};

struct Slider
{
    static constexpr auto miroTag = 2;
    double value = 0.0;
    MIRO_REFLECT(value)
};

} // namespace

// External reflect: a free `reflect(Reflector&, T&)` found by ADL on T's
// namespace. File-scope namespace so the lookup is the realistic one.
namespace external
{
struct Point
{
    int x = 0;
    int y = 0;
};

inline void reflect(Miro::Reflector& ref, Point& value)
{
    ref["x"](value.x);
    ref["y"](value.y);
}
} // namespace external

// A user primitive taught to Miro after the fact, the juce::String /
// QString pattern from UserPrimitiveTests.cpp. The concept has to agree
// with the dispatcher, which does find this late overload.
namespace user
{
struct UserString
{
    std::string data;
};
} // namespace user

namespace Miro
{
inline void reflectValue(Reflector& ref, user::UserString& value)
{
    ref.visit(value.data);
}
} // namespace Miro

// --- Primitives ---------------------------------------------------------

static_assert(Reflectable<bool>);
static_assert(Reflectable<int>);
static_assert(Reflectable<double>);
static_assert(Reflectable<std::string>);
static_assert(Reflectable<std::int64_t>);

// The widening integral / floating overloads cover the rest of the
// arithmetic family.
static_assert(Reflectable<float>);
static_assert(Reflectable<short>);
static_assert(Reflectable<unsigned>);
static_assert(Reflectable<std::uint8_t>);
static_assert(Reflectable<std::uint64_t>);
static_assert(Reflectable<long long>);

// --- Enums --------------------------------------------------------------

static_assert(Reflectable<Scoped>);
static_assert(Reflectable<Unscoped>);

// --- Standard containers and wrappers -----------------------------------

static_assert(Reflectable<std::vector<int>>);
static_assert(Reflectable<std::array<int, 3>>);
static_assert(Reflectable<std::map<std::string, int>>);
static_assert(Reflectable<std::optional<int>>);
static_assert(Reflectable<Omittable<int>>);
static_assert(Reflectable<std::variant<int, std::string>>);

// --- EA containers ------------------------------------------------------

static_assert(Reflectable<Vector<int>>);
static_assert(Reflectable<Array<int, 3>>);
static_assert(Reflectable<EA::MapVector<std::string, int>>);
static_assert(Reflectable<OwningPointer<Inner>>);
static_assert(Reflectable<Variant<int, std::string>>);

// --- Raw JSON -----------------------------------------------------------

static_assert(Reflectable<JSON>);
static_assert(Reflectable<Json::Value>);
static_assert(Reflectable<Json::Any>);

// --- User types ---------------------------------------------------------

static_assert(Reflectable<Inner>);
static_assert(Reflectable<Outer>);
static_assert(Reflectable<MacroStruct>);
static_assert(Reflectable<external::Point>);
static_assert(Reflectable<ConstReflect>);
static_assert(Reflectable<user::UserString>);

// --- Tagged unions ------------------------------------------------------

static_assert(Reflectable<Polymorphic<ShapeBase, Circle, Square>>);
static_assert(Reflectable<TaggedVariant<"type", Button, Slider>>);
static_assert(Reflectable<Tagged<"type", ShapeBase, Circle, Square>>);

// --- Nesting ------------------------------------------------------------

static_assert(Reflectable<std::vector<Inner>>);
static_assert(Reflectable<std::vector<std::vector<int>>>);
static_assert(Reflectable<std::optional<std::vector<Inner>>>);
static_assert(Reflectable<std::map<std::string, std::vector<int>>>);
static_assert(Reflectable<Omittable<std::optional<Inner>>>);
static_assert(Reflectable<std::array<std::map<std::string, Scoped>, 2>>);
static_assert(Reflectable<std::vector<external::Point>>);
static_assert(Reflectable<std::vector<user::UserString>>);

// --- Not reflectable ----------------------------------------------------

static_assert(!Reflectable<Plain>);
static_assert(!Reflectable<WrongReflectSignature>);

static_assert(!Reflectable<std::string_view>);
static_assert(!Reflectable<const char*>);
static_assert(!Reflectable<char*>);
static_assert(!Reflectable<void*>);
static_assert(!Reflectable<std::nullptr_t>);

// Only string-keyed maps have a JSON shape.
static_assert(!Reflectable<std::map<int, int>>);

static_assert(!Reflectable<std::set<int>>);
static_assert(!Reflectable<std::list<int>>);
static_assert(!Reflectable<std::pair<int, int>>);
static_assert(!Reflectable<std::tuple<int, int>>);
static_assert(!Reflectable<std::unique_ptr<int>>);
static_assert(!Reflectable<std::shared_ptr<Inner>>);
static_assert(!Reflectable<std::function<void()>>);

auto reflectableConceptCompiles =
    test("Reflectable concept static_asserts compile") = []
{
    check(Reflectable<Inner>);
    check(!Reflectable<Plain>);
};
