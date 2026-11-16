#include <Miro/Reflect.h>

#include <NanoTest/NanoTest.h>

#include <string_view>

using namespace nano;

struct TypeIdPlain
{
    MIRO_REFLECT(value)

    int value = 0;
};

namespace TypeIdOuter::Inner
{
struct Widget
{
    MIRO_REFLECT(size)

    int size = 0;
};
} // namespace TypeIdOuter::Inner

namespace
{
struct NamedTypeRecorder : Miro::Reflector
{
    Miro::TypeId announced {};

    NamedTypeRecorder()
        : Reflector({.shape = Miro::Shape::Object})
    {
    }

    bool beginNamedType(Miro::TypeId id) override
    {
        announced = id;
        return false;
    }

    void visit(Miro::PrimitiveRef) override {}
    void writeNull() override {}
    Miro::ValueKind kind() const override { return Miro::ValueKind::Absent; }
    Reflector& atKey(std::string_view, Miro::Options) override { return *this; }
    Reflector& atIndex(std::size_t, Miro::Options) override { return *this; }
};
} // namespace

static_assert(Miro::typeIdOf<TypeIdPlain>().shortName == "TypeIdPlain");

auto typeIdOfGlobalType = test("typeIdOf names a global type") = []
{
    auto id = Miro::typeIdOf<TypeIdPlain>();

    check(id.shortName == "TypeIdPlain");
    check(id.qualifiedName == "TypeIdPlain");
};

auto typeIdOfNamespacedType = test("typeIdOf names a namespaced type") = []
{
    auto id = Miro::typeIdOf<TypeIdOuter::Inner::Widget>();

    check(id.shortName == "Widget");
    check(id.qualifiedName == "TypeIdOuter::Inner::Widget");
};

auto typeIdOfMatchesDispatch = test("typeIdOf matches the announced TypeId") = []
{
    auto recorder = NamedTypeRecorder {};
    auto widget = TypeIdOuter::Inner::Widget {};
    recorder["widget"](widget);

    auto expected = Miro::typeIdOf<TypeIdOuter::Inner::Widget>();
    check(recorder.announced.shortName == expected.shortName);
    check(recorder.announced.qualifiedName == expected.qualifiedName);
};
