#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

namespace
{
struct EchoRequest
{
    std::string text;

    MIRO_REFLECT(text)
};

struct EchoResponse
{
    std::string echoed;

    MIRO_REFLECT(echoed)
};

EchoResponse handleEcho(const EchoRequest& req)
{
    return EchoResponse {.echoed = req.text + "!"};
}

struct PingResponse
{
    bool pong = true;

    MIRO_REFLECT(pong)
};

PingResponse handlePing(const EmptyValue&)
{
    return PingResponse {.pong = true};
}

PingResponse handlePingNoArg()
{
    return PingResponse {.pong = true};
}

int kickCount = 0;

void handleSetCount(const EchoRequest& req)
{
    kickCount = static_cast<int>(req.text.size());
}

void handleKick()
{
    kickCount += 1;
}
} // namespace

auto dispatchTyped = test("CommandTable dispatches typed handler") = []
{
    auto table = CommandTable {};
    table.on("echo", handleEcho);

    auto payload = Json::parse(R"({"text":"hi"})");
    auto result = table.dispatch("echo", payload);

    check(result.isObject());
    check(result["echoed"].asString() == "hi!");
};

auto dispatchEmpty = test("CommandTable handles EmptyValue input") = []
{
    auto table = CommandTable {};
    table.on("ping", handlePing);

    auto result = table.dispatch("ping", JSON {});

    check(result.isObject());
    check(result["pong"].asBool() == true);
};

auto dispatchUnknown = test("CommandTable throws on unknown command") = []
{
    auto table = CommandTable {};
    auto threw = false;

    try
    {
        table.dispatch("missing", JSON {});
    }
    catch (const UnknownCommandError&)
    {
        threw = true;
    }

    check(threw);
};

auto dispatchHas = test("CommandTable::has reflects registration") = []
{
    auto table = CommandTable {};
    check(!table.has("echo"));

    table.on("echo", handleEcho);
    check(table.has("echo"));
};

auto dispatchNoArg = test("CommandTable dispatches no-arg handler") = []
{
    auto table = CommandTable {};
    table.on("ping", handlePingNoArg);

    auto result = table.dispatch("ping", JSON {});

    check(result.isObject());
    check(result["pong"].asBool() == true);
};

auto dispatchNoArgIgnoresPayload =
    test("CommandTable no-arg handler ignores incoming payload") = []
{
    auto table = CommandTable {};
    table.on("ping", handlePingNoArg);

    auto payload = Json::parse(R"({"unused":"ignored"})");
    auto result = table.dispatch("ping", payload);

    check(result["pong"].asBool() == true);
};

auto dispatchVoidReturn =
    test("CommandTable dispatches void-returning handler with request") = []
{
    auto table = CommandTable {};
    table.on("setCount", handleSetCount);

    kickCount = 0;
    auto payload = Json::parse(R"({"text":"abcd"})");
    auto result = table.dispatch("setCount", payload);

    check(result.isNull());
    check(kickCount == 4);
};

auto dispatchVoidNoArg = test("CommandTable dispatches void no-arg handler") = []
{
    auto table = CommandTable {};
    table.on("kick", handleKick);

    kickCount = 0;
    auto result = table.dispatch("kick", JSON {});

    check(result.isNull());
    check(kickCount == 1);
};
