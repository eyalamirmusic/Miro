#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

using namespace nano;

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

PingResponse handlePing(const Miro::EmptyValue&)
{
    return PingResponse {.pong = true};
}
} // namespace

auto dispatchTyped = test("CommandTable dispatches typed handler") = []
{
    auto table = Miro::CommandTable {};
    table.on("echo", handleEcho);

    auto payload = Miro::Json::parse(R"({"text":"hi"})");
    auto result = table.dispatch("echo", payload);

    check(result.isObject());
    check(result["echoed"].asString() == "hi!");
};

auto dispatchEmpty = test("CommandTable handles EmptyValue input") = []
{
    auto table = Miro::CommandTable {};
    table.on("ping", handlePing);

    auto result = table.dispatch("ping", Miro::Json::Value {});

    check(result.isObject());
    check(result["pong"].asBool() == true);
};

auto dispatchUnknown = test("CommandTable throws on unknown command") = []
{
    auto table = Miro::CommandTable {};
    auto threw = false;

    try
    {
        table.dispatch("missing", Miro::Json::Value {});
    }
    catch (const Miro::UnknownCommandError&)
    {
        threw = true;
    }

    check(threw);
};

auto dispatchHas = test("CommandTable::has reflects registration") = []
{
    auto table = Miro::CommandTable {};
    check(!table.has("echo"));

    table.on("echo", handleEcho);
    check(table.has("echo"));
};
