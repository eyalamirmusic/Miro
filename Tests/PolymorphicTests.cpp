// Coverage for polymorphic reflection: std::variant<Ts...>,
// Miro::Polymorphic<Base, Derived...>, the reflectPolymorphic callback
// API, the MIRO_REFLECT_POLY / MIRO_REFLECT_EXTERNAL_POLY macros, and
// the assertion hook fired by schema-mode reflectors.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <stdexcept>
#include <string>
#include <variant>

using namespace nano;
using namespace Miro;

namespace
{

struct Circle
{
    double radius = 0.0;
    MIRO_REFLECT(radius)
};

struct Square
{
    double side = 0.0;
    MIRO_REFLECT(side)
};

struct Triangle
{
    double base = 0.0;
    double height = 0.0;
    MIRO_REFLECT(base, height)
};

// Base class for the OwningPointer<Base> / Polymorphic<...> tests. Needs
// a virtual destructor so dynamic_cast works and OwningPointer's delete
// is well-formed; an empty reflect() satisfies the Reflectable concept
// (the dispatcher never calls it directly — alternatives are always
// reflected through their concrete derived reflect()).
struct ShapeBase
{
    virtual ~ShapeBase() = default;
    void reflect(Miro::Reflector&) {}
};

struct CircleShape : ShapeBase
{
    double radius = 0.0;
    MIRO_REFLECT(radius)
};

struct SquareShape : ShapeBase
{
    double side = 0.0;
    MIRO_REFLECT(side)
};

// Used by the "MIRO_REFLECT_POLY" intrusive-macro test.
struct PolyVariantField
{
    std::variant<Circle, Square> value;
    MIRO_REFLECT_POLY(value, Circle, "circle", Square, "square")
};

} // namespace

// File-scope (not anonymous) so MIRO_REFLECT_EXTERNAL_POLY's generated
// reflect() in namespace Miro doesn't pick up internal linkage from an
// anonymous-namespace parameter type, which would trip -Wunused-function
// on this TU.
struct ExternalPolyType
{
    std::variant<Circle, Square> shape;
};

MIRO_REFLECT_EXTERNAL_POLY(
    ExternalPolyType, shape, Circle, "circle", Square, "square")

namespace
{

// Nested-field test: a normal struct that owns a polymorphic field
// alongside a regular one. Validates the externally-tagged shape
// nests correctly under a JSON key.
struct Drawing
{
    std::string name;
    Polymorphic<ShapeBase, CircleShape, SquareShape> shape {};

    MIRO_REFLECT(name, shape)
};

// Hand-written reflect() for the "unknown tag on load" test. Exposes
// the dispatcher's handled() result through a member so the test can
// observe it without a side channel.
struct PolyWithHandledFlag
{
    std::variant<Circle, Square> value;
    mutable bool lastLoadHandled = false;

    void reflect(Miro::Reflector& ref)
    {
        auto dispatcher =
            Miro::PolymorphicDispatcher<std::variant<Circle, Square>> {ref, value};
        dispatcher.template alt<Circle>("circle");
        dispatcher.template alt<Square>("square");
        lastLoadHandled = dispatcher.handled();
    }
};

} // namespace

auto variantRoundTripCircle = test("variant<Circle, Square> round-trips Circle") = []
{
    auto original = std::variant<Circle, Square> {Circle {2.5}};
    auto json = toJSONString(original);

    check(json == R"({"Circle":{"radius":2.5}})");

    auto loaded = createFromJSONString<std::variant<Circle, Square>>(json);

    check(std::holds_alternative<Circle>(loaded));
    check(std::get<Circle>(loaded).radius == 2.5);
};

auto variantRoundTripSquare = test("variant<Circle, Square> round-trips Square") = []
{
    auto original = std::variant<Circle, Square> {Square {4.0}};
    auto json = toJSONString(original);

    check(json == R"({"Square":{"side":4}})");

    auto loaded = createFromJSONString<std::variant<Circle, Square>>(json);

    check(std::holds_alternative<Square>(loaded));
    check(std::get<Square>(loaded).side == 4.0);
};

auto variantOfPrimitiveCustomTags =
    test("variant of primitives with custom tags via reflectPolymorphic") = []
{
    struct Wrapper
    {
        std::variant<int, std::string> value;

        void reflect(Miro::Reflector& ref)
        {
            Miro::reflectPolymorphic(ref,
                                     value,
                                     [](auto& d)
                                     {
                                         d.template alt<int>("int");
                                         d.template alt<std::string>("string");
                                     });
        }
    };

    auto intHolder = Wrapper {.value = 42};
    auto intJson = toJSONString(intHolder);
    check(intJson == R"({"int":42})");
    auto intLoaded = createFromJSONString<Wrapper>(intJson);
    check(std::holds_alternative<int>(intLoaded.value));
    check(std::get<int>(intLoaded.value) == 42);

    auto stringHolder = Wrapper {.value = std::string {"hello"}};
    auto stringJson = toJSONString(stringHolder);
    check(stringJson == R"({"string":"hello"})");
    auto stringLoaded = createFromJSONString<Wrapper>(stringJson);
    check(std::holds_alternative<std::string>(stringLoaded.value));
    check(std::get<std::string>(stringLoaded.value) == "hello");
};

auto polymorphicHolderCircleRoundTrip =
    test("Polymorphic<ShapeBase, CircleShape, SquareShape> round-trips Circle") = []
{
    auto original = Polymorphic<ShapeBase, CircleShape, SquareShape> {};
    auto* derived = original.value.create<CircleShape>();
    derived->radius = 1.25;

    auto json = toJSONString(original);
    check(json == R"({"CircleShape":{"radius":1.25}})");

    auto loaded =
        createFromJSONString<Polymorphic<ShapeBase, CircleShape, SquareShape>>(json);

    check(loaded.value.get() != nullptr);
    auto* circle = dynamic_cast<CircleShape*>(loaded.value.get());
    check(circle != nullptr);
    check(circle->radius == 1.25);
};

auto polymorphicHolderSquareRoundTrip =
    test("Polymorphic<ShapeBase, CircleShape, SquareShape> round-trips Square") = []
{
    auto original = Polymorphic<ShapeBase, CircleShape, SquareShape> {};
    auto* derived = original.value.create<SquareShape>();
    derived->side = 7.5;

    auto json = toJSONString(original);
    check(json == R"({"SquareShape":{"side":7.5}})");

    auto loaded =
        createFromJSONString<Polymorphic<ShapeBase, CircleShape, SquareShape>>(json);

    check(loaded.value.get() != nullptr);
    auto* square = dynamic_cast<SquareShape*>(loaded.value.get());
    check(square != nullptr);
    check(square->side == 7.5);
};

auto reflectPolymorphicCustomTags =
    test("reflectPolymorphic with custom lowercase tags round-trips") = []
{
    struct LowerCaseTagged
    {
        std::variant<Circle, Square> value;

        void reflect(Miro::Reflector& ref)
        {
            Miro::reflectPolymorphic(ref,
                                     value,
                                     [](auto& d)
                                     {
                                         d.template alt<Circle>("circle");
                                         d.template alt<Square>("square");
                                     });
        }
    };

    auto holder = LowerCaseTagged {.value = Square {3.0}};
    auto json = toJSONString(holder);
    check(json == R"({"square":{"side":3}})");

    auto loaded = createFromJSONString<LowerCaseTagged>(json);
    check(std::holds_alternative<Square>(loaded.value));
    check(std::get<Square>(loaded.value).side == 3.0);
};

auto reflectPolyMacroRoundTrip =
    test("MIRO_REFLECT_POLY round-trips with lowercase tags") = []
{
    auto holder = PolyVariantField {.value = Circle {9.0}};
    auto json = toJSONString(holder);

    check(json == R"({"circle":{"radius":9}})");

    auto loaded = createFromJSONString<PolyVariantField>(json);
    check(std::holds_alternative<Circle>(loaded.value));
    check(std::get<Circle>(loaded.value).radius == 9.0);
};

auto reflectExternalPolyMacroRoundTrip =
    test("MIRO_REFLECT_EXTERNAL_POLY round-trips a non-intrusive type") = []
{
    auto holder = ExternalPolyType {.shape = Square {6.0}};
    auto json = toJSONString(holder);

    check(json == R"({"square":{"side":6}})");

    auto loaded = createFromJSONString<ExternalPolyType>(json);
    check(std::holds_alternative<Square>(loaded.shape));
    check(std::get<Square>(loaded.shape).side == 6.0);
};

auto unknownTagOnLoadLeavesValueAlone =
    test("unknown tag on load leaves value untouched and reports !handled") = []
{
    auto holder = PolyWithHandledFlag {.value = Circle {0.0}};

    // Save and confirm the round trip baseline works.
    auto json = toJSONString(holder);
    check(json == R"({"circle":{"radius":0}})");

    // Now load a payload carrying a tag we didn't register.
    fromJSONString(holder, R"({"triangle":{"base":1,"height":2}})");

    check(!holder.lastLoadHandled);
    // The variant should still hold a Circle (its prior state) — no
    // alternative matched, so assign<T>() never fired.
    check(std::holds_alternative<Circle>(holder.value));
};

auto nestedPolymorphicFieldRoundTrip =
    test("Polymorphic field nested in a regular struct round-trips") = []
{
    auto drawing = Drawing {.name = "first"};
    auto* derived = drawing.shape.value.create<CircleShape>();
    derived->radius = 11.0;

    auto json = toJSONString(drawing);
    check(json == R"({"name":"first","shape":{"CircleShape":{"radius":11}}})");

    auto loaded = createFromJSONString<Drawing>(json);
    check(loaded.name == "first");
    check(loaded.shape.value.get() != nullptr);
    auto* circle = dynamic_cast<CircleShape*>(loaded.shape.value.get());
    check(circle != nullptr);
    check(circle->radius == 11.0);
};

auto xmlVariantRoundTrip =
    test("variant nested in a named struct round-trips through XML") = []
{
    // Top-level XML uses the type's short name as the root element name,
    // which produces invalid XML for unnamed/templated types — so we
    // wrap the variant in a regular struct (the realistic use case).
    auto original = PolyVariantField {.value = Square {2.0}};
    auto xmlString = toXMLString(original);

    auto loaded = createFromXMLString<PolyVariantField>(xmlString);
    check(std::holds_alternative<Square>(loaded.value));
    check(std::get<Square>(loaded.value).side == 2.0);
};

auto schemaOfPolymorphicThrows =
    test("schemaOf<variant> throws std::logic_error (codegen out of scope)") = []
{
    auto threw = false;
    try
    {
        (void) Miro::schemaOf<std::variant<Circle, Square>>();
    }
    catch (const std::logic_error&)
    {
        threw = true;
    }
    check(threw);
};
