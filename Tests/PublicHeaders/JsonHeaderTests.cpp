// Self-containment check: <Miro/Json.h> must compile as the first and
// only Miro include. The test body is a minimal smoke of the raw JSON
// layer; the real assertion is that this TU compiles at all.

#include <Miro/Json.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

auto jsonEntryHeader = test("Entry header: Miro/Json.h is self-contained") = []
{
    auto value = Miro::Json::parse(R"({"answer": 42})");
    check(value["answer"].asNumber() == 42.0);
};
