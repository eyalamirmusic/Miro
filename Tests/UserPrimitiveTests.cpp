// Out-of-tree primitive extension: users should be able to teach Miro how
// to serialize a type they don't control (e.g. juce::String, QString) as a
// string slot, by adding a `reflectValue` overload after including
// <Miro/Miro.h>. The dispatcher must find the late overload when
// Property::operator()<T> is instantiated for a reflecting type that owns
// a field of that type.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

namespace user
{
// A string-like primitive defined outside Miro and outside std. Stand-in
// for juce::String / QString.
struct UserString
{
    std::string data;
};
} // namespace user

// User-supplied overload added AFTER <Miro/Miro.h>. Placed in namespace
// Miro (Reflector's namespace) so ADL on the first argument can find it
// at template instantiation. Property::operator()'s dispatch has to use
// unqualified invocation for this to work — qualified `Detail::reflectValue`
// would freeze the candidate set at template definition.
namespace Miro
{
inline void reflectValue(Reflector& ref, user::UserString& value)
{
    auto buffer = std::string {};

    if (ref.isSaving())
        buffer = value.data;

    ref.visit(buffer);

    if (ref.isLoading())
        value.data = buffer;
}
} // namespace Miro

namespace
{
struct WithUserString
{
    user::UserString label;

    MIRO_REFLECT(label)
};
} // namespace

auto saveUserPrimitive = test("Save user-defined primitive via late overload") = []
{
    auto value = WithUserString {};
    value.label.data = "hello";

    auto json = toJSON(value);

    check(json["label"].isString());
    check(json["label"].asString() == "hello");
};

auto loadUserPrimitive = test("Load user-defined primitive via late overload") = []
{
    auto value = createFromJSONString<WithUserString>(R"({"label": "world"})");

    check(value.label.data == "world");
};
