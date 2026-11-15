// Self-containment check: <Miro/OpenAI.h> must compile as the first
// and only Miro include, and must supply the OpenAI tool layer.

#include <Miro/OpenAI.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

namespace
{
struct PingApi
{
    int ping() { return 1; }

    void reflect(Miro::ApiReflector& r)
    {
        using T = PingApi;
        r.command<&T::ping>();
    }
};
} // namespace

auto openAiEntryHeader = test("Entry header: Miro/OpenAI.h is self-contained") = []
{
    auto api = PingApi {};
    auto tools = Miro::OpenAI::ToolSet::describe(api);
    check(tools.tools().size() == 1);
    check(tools.definitions()[0]["name"].asString() == "ping");
};
