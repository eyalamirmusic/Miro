// The real assertion is that this TU compiles with one Miro include.

#include <Miro/Xml.h>

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

auto xmlEntryHeader = test("Entry header: Miro/Xml.h is self-contained") = []
{
    auto point = Point {};
    point.x = 3;

    auto copy = Miro::createFromXMLString<Point>(Miro::toXMLString(point));
    check(copy.x == 3);
    check(copy.y == 0);
};
