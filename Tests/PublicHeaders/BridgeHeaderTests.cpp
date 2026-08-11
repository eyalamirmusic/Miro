// The real assertion is that this TU compiles with one Miro include.

#include <Miro/Bridge.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

auto bridgeEntryHeader = test("Entry header: Miro/Bridge.h is self-contained") = []
{
    auto bridge = Miro::Bridge {};
    bridge.on("answer", +[] { return 42; });

    check(bridge.dispatch("answer", Miro::JSON {}).asNumber() == 42.0);
};
