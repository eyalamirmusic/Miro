#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

namespace user
{
struct UserString
{
    std::string data;
};
} // namespace user

// Deliberately declared after <Miro/Miro.h> and inside namespace Miro: only
// ADL at instantiation time finds it, and only because Property::operator()
// calls reflectValue unqualified.
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
