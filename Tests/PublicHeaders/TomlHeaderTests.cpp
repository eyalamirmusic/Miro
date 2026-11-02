// Self-containment check: <Miro/Toml.h> must compile as the first and
// only Miro include, and must supply the TOML parse / print plus the
// toTOMLString / fromTOMLString reflection serializers.

#include <Miro/Toml.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

namespace
{
struct Point
{
    void reflect(Miro::Reflector& r)
    {
        r["x"](x);
        r["y"](y);
    }

    int x = 0;
    int y = 0;
};
} // namespace

auto tomlEntryHeader = test("Entry header: Miro/Toml.h is self-contained") = []
{
    auto point = Point {};
    point.x = 3;

    auto copy = Miro::createFromTOMLString<Point>(Miro::toTOMLString(point));
    check(copy.x == 3);
    check(copy.y == 0);
};
