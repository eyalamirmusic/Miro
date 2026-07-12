// Self-containment check: <Miro/Bridge.h> must compile as the first
// and only Miro include, and must supply the runtime bridge surface.

#include <Miro/Bridge.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

auto bridgeEntryHeader = test("Entry header: Miro/Bridge.h is self-contained") = []
{
    auto bridge = Miro::Bridge {};
    bridge.on("answer", +[] { return 42; });

    check(bridge.dispatch("answer", Miro::JSON {}).asNumber() == 42.0);
};
