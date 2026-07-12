// Self-containment check: <Miro/Reflect.h> must compile as the first
// and only Miro include, and must supply MIRO_REFLECT plus the JSON
// serialization helpers.

#include <Miro/Reflect.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

namespace
{
struct Size
{
    MIRO_REFLECT(width, height)

    int width = 0;
    int height = 0;
};
} // namespace

auto reflectEntryHeader = test("Entry header: Miro/Reflect.h is self-contained") = []
{
    auto size = Size {};
    size.width = 5;

    auto copy = Miro::createFromJSONString<Size>(Miro::toJSONString(size));
    check(copy.width == 5);
    check(copy.height == 0);
};
