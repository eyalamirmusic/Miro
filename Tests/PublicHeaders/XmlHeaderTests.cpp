// Self-containment check: <Miro/Xml.h> must compile as the first and
// only Miro include, and must supply the XML value layer plus the
// toXML / fromXML reflection serializers.

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
