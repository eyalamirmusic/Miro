// The real assertion is that this TU compiles with one Miro include.

#include <Miro/Json.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

auto jsonEntryHeader = test("Entry header: Miro/Json.h is self-contained") = []
{
    auto value = Miro::Json::parse(R"({"answer": 42})");
    check(value["answer"].asNumber() == 42.0);
};
