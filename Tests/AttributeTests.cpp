// Field attributes: values passed beside a field, ref["speed"](speed,
// Range {0.f, 10.f}, Unit::hertz), that reach the child slot through one
// Reflector::visitAttributes call before the value is visited. A
// reflector that doesn't override it sees nothing (JSON is unchanged);
// TypeTree keeps Doc as the description and every reflectable attribute
// as JSON under its qualified type name.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

// User-defined attributes Miro knows nothing about.
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

// Not reflectable: Miro can't write a function pointer out, but a live
// reflector can still read it.
struct OnChange
{
    void (*callback)(int) = nullptr;
};

// Never attached to anything; as<UnusedAttribute>() must stay null.
struct UnusedAttribute
{
    int value = 0;
};

void onCountChanged(int) {}

} // namespace

// The applicability hook: a Range only makes sense on a number. It has
// to be visible before the first field that uses a Range. Passing one on
// a std::string field would fail the static_assert in
// Property::operator(), which a test that has to compile can't
// exercise, so the trait itself is checked below.
namespace Miro
{
template <typename T>
struct AttributeAppliesTo<Range, T>
{
    static constexpr bool value = std::is_arithmetic_v<T>;
};
} // namespace Miro

namespace
{

struct PlainFields
{
    float speed = 2.5f;
    std::string city = "Paris";
    int count = 7;

    void reflect(Reflector& ref)
    {
        ref["speed"](speed);
        ref["city"](city);
        ref["count"](count);
    }
};

struct AttributedFields
{
    float speed = 2.5f;
    std::string city = "Paris";
    int count = 7;

    void reflect(Reflector& ref)
    {
        ref["speed"](speed, Range {0.f, 10.f}, Unit::hertz);
        ref["city"](city, Doc {"City name"});
        ref["count"].doc("How many")(
            count, Range {0.f, 100.f}, OnChange {&onCountChanged});
    }
};

struct DocViaMethod
{
    std::string city;

    void reflect(Reflector& ref) { ref["city"].doc("City and country")(city); }
};

struct DocViaAttribute
{
    std::string city;

    void reflect(Reflector& ref) { ref["city"](city, Doc {"City and country"}); }
};

// What the recording reflector saw, shared by every slot of one walk.
struct AttributeLog
{
    std::vector<std::string> events;
    std::vector<Range> ranges;
    std::vector<Unit> units;
    int unusedMatches = 0;
    void (*callback)(int) = nullptr;
};

// One slot of a walk that logs attribute deliveries and value visits in
// order. Attribute views are only valid during visitAttributes, so the
// typed payloads are copied out there.
struct RecordingReflector : Reflector
{
    RecordingReflector(AttributeLog& logToUse,
                       Options optsToUse,
                       std::string labelToUse = {})
        : Reflector(optsToUse)
        , log(logToUse)
        , label(std::move(labelToUse))
    {
    }

    void visitAttributes(std::span<const AttributeRef> attributes) override
    {
        for (auto& attribute: attributes)
        {
            log.events.push_back(label + "@" + std::string {attribute.typeName});

            if (auto* range = attribute.as<Range>())
                log.ranges.push_back(*range);

            if (auto* unit = attribute.as<Unit>())
                log.units.push_back(*unit);

            if (attribute.as<UnusedAttribute>() != nullptr)
                ++log.unusedMatches;

            if (auto* onChange = attribute.as<OnChange>())
                log.callback = onChange->callback;
        }
    }

    void visit(PrimitiveRef) override { log.events.push_back(label + "=value"); }
    void writeNull() override {}
    ValueKind kind() const override { return ValueKind::Absent; }

    Reflector& atKey(std::string_view key, Options childOpts) override
    {
        currentChild =
            std::make_unique<RecordingReflector>(log, childOpts, std::string {key});
        return *currentChild;
    }

    Reflector& atIndex(std::size_t index, Options childOpts) override
    {
        currentChild = std::make_unique<RecordingReflector>(
            log, childOpts, "[" + std::to_string(index) + "]");
        return *currentChild;
    }

    AttributeLog& log;
    std::string label;
    std::unique_ptr<RecordingReflector> currentChild;
};

std::string attributeEvent(std::string_view label, std::string_view typeName)
{
    return std::string {label} + "@" + std::string {typeName};
}

const TypeTree::TypeNode& attributeFieldNode(const TypeTree::TypeNode& node,
                                             std::string_view name)
{
    for (auto& field: node.fields)
        if (field.name == name)
            return *field.type;

    throw std::runtime_error("no field " + std::string {name});
}

} // namespace

static_assert(AttributeAppliesTo<Range, float>::value);
static_assert(AttributeAppliesTo<Range, int>::value);
static_assert(!AttributeAppliesTo<Range, std::string>::value);
static_assert(AttributeAppliesTo<Doc, std::string>::value);

auto attributesReachChildSlot =
    test("Attributes: delivered on the child slot, in order, before the value") = []
{
    auto log = AttributeLog {};
    auto reflector = RecordingReflector {log, {.shape = Shape::Object}};
    auto value = AttributedFields {};

    value.reflect(reflector);

    auto range = Detail::qualifiedNameOf<Range>();
    auto unit = Detail::qualifiedNameOf<Unit>();
    auto doc = Detail::qualifiedNameOf<Doc>();
    auto onChange = Detail::qualifiedNameOf<OnChange>();

    auto expected = std::vector<std::string> {
        attributeEvent("speed", range),
        attributeEvent("speed", unit),
        "speed=value",
        attributeEvent("city", doc),
        "city=value",
        attributeEvent("count", doc),
        attributeEvent("count", range),
        attributeEvent("count", onChange),
        "count=value",
    };

    check(log.events == expected);
    check(doc == "Miro::Doc");
};

auto attributesTypedAccess = test("Attributes: as<A>() returns the typed value") = []
{
    auto log = AttributeLog {};
    auto reflector = RecordingReflector {log, {.shape = Shape::Object}};
    auto value = AttributedFields {};

    value.reflect(reflector);

    check(log.ranges.size() == 2);
    check(log.ranges[0].min == 0.f);
    check(log.ranges[0].max == 10.f);
    check(log.ranges[1].max == 100.f);
    check(log.units.size() == 1);
    check(log.units[0] == Unit::hertz);
    check(log.callback == &onCountChanged);
    check(log.unusedMatches == 0);
};

auto attributeRefDirect = test("Attributes: AttributeRef identity and access") = []
{
    auto range = Range {1.f, 2.f};
    auto ref = AttributeRef::of(range);

    check(ref.typeName == Detail::qualifiedNameOf<Range>());
    check(ref.as<Range>() == &range);
    check(ref.as<UnusedAttribute>() == nullptr);
    check(ref.isReflectable());

    auto onChange = OnChange {&onCountChanged};
    check(!AttributeRef::of(onChange).isReflectable());
    check(!AttributeRef::of(Doc {"x"}).isReflectable());
};

auto attributesNoDocNoCall =
    test("Attributes: a plain field never calls visitAttributes") = []
{
    auto log = AttributeLog {};
    auto reflector = RecordingReflector {log, {.shape = Shape::Object}};
    auto value = PlainFields {};

    value.reflect(reflector);

    auto expected =
        std::vector<std::string> {"speed=value", "city=value", "count=value"};
    check(log.events == expected);
};

auto attributesJsonUnchanged =
    test("Attributes: JSON is identical with and without attributes") = []
{
    check(toJSONString(AttributedFields {}) == toJSONString(PlainFields {}));

    auto original = AttributedFields {};
    original.speed = 4.f;
    original.city = "Lyon";
    original.count = 42;

    auto text = toJSONString(original);
    auto plain = PlainFields {4.f, "Lyon", 42};
    check(text == toJSONString(plain));

    auto restored = createFromJSONString<AttributedFields>(text);
    check(restored.speed == 4.f);
    check(restored.city == "Lyon");
    check(restored.count == 42);
};

auto attributesDocSpellings =
    test("Attributes: .doc() and Doc {} give the same description") = []
{
    auto viaMethod = TypeTree::buildTree<DocViaMethod>();
    auto viaAttribute = TypeTree::buildTree<DocViaAttribute>();

    auto& methodCity = attributeFieldNode(viaMethod, "city");
    auto& attributeCity = attributeFieldNode(viaAttribute, "city");

    check(methodCity.description == "City and country");
    check(attributeCity.description == "City and country");
    check(methodCity.attributes.empty());
    check(attributeCity.attributes.empty());
};

auto attributesRetainedOnTypeNode =
    test("Attributes: reflectable ones are kept on TypeNode as JSON") = []
{
    auto tree = TypeTree::buildTree<AttributedFields>();

    auto rangeName = std::string {Detail::qualifiedNameOf<Range>()};
    auto unitName = std::string {Detail::qualifiedNameOf<Unit>()};

    auto& speed = attributeFieldNode(tree, "speed");
    check(speed.description.empty());
    check(speed.attributes.size() == 2);
    check(speed.attributes.at(rangeName).asObject().at("min").asNumber() == 0.0);
    check(speed.attributes.at(rangeName).asObject().at("max").asNumber() == 10.0);
    check(speed.attributes.at(unitName).asString() == "hertz");

    auto& city = attributeFieldNode(tree, "city");
    check(city.description == "City name");
    check(city.attributes.empty());

    // Doc goes to the description; the function-pointer attribute is
    // not reflectable, so it is left out.
    auto& count = attributeFieldNode(tree, "count");
    check(count.description == "How many");
    check(count.attributes.size() == 1);
    check(count.attributes.at(rangeName).asObject().at("max").asNumber() == 100.0);
    check(!count.attributes.contains(
        std::string {Detail::qualifiedNameOf<OnChange>()}));

    check(tree.attributes.empty());
};
