#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

#include <functional>
#include <future>
#include <thread>

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
    return {.echoed = req.text + "!"};
}

struct PingResponse
{
    bool pong = true;

    MIRO_REFLECT(pong)
};

PingResponse handlePing(const EmptyValue&)
{
    return {.pong = true};
}

PingResponse handlePingNoArg()
{
    return {.pong = true};
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

    auto result = table.dispatch("ping", {});

    check(result.isObject());
    check(result["pong"].asBool() == true);
};

auto dispatchUnknown = test("CommandTable throws on unknown command") = []
{
    auto table = CommandTable {};
    auto threw = false;

    try
    {
        table.dispatch("missing", {});
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

    auto result = table.dispatch("ping", {});

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
    auto result = table.dispatch("kick", {});

    check(result.isNull());
    check(kickCount == 1);
};

// ---------- async (Completer) handlers ----------

namespace
{
struct AsyncOutcome
{
    JSON result;
    bool hasError = false;
    std::string error;
    int settleCount = 0;
};

// A Resolve that records what (and how often) it was settled with.
Resolve recordInto(AsyncOutcome& outcome)
{
    return [&outcome](const JSON& result, const std::string* error)
    {
        outcome.settleCount += 1;

        if (error != nullptr)
        {
            outcome.hasError = true;
            outcome.error = *error;
        }
        else
        {
            outcome.result = result;
        }
    };
}
} // namespace

auto asyncInline = test("CommandTable onAsync resolves inline") = []
{
    auto table = CommandTable {};
    table.onAsync<EchoRequest, EchoResponse>(
        "echo",
        [](const EchoRequest& req, Completer<EchoResponse> done)
        { done.resolve({.echoed = req.text + "!"}); });

    auto outcome = AsyncOutcome {};
    table.dispatchAsync(
        "echo", Json::parse(R"({"text":"hi"})"), recordInto(outcome));

    check(!outcome.hasError);
    check(outcome.result["echoed"].asString() == "hi!");
};

auto asyncWorkerThread =
    test("CommandTable onAsync resolves later from a worker thread") = []
{
    auto table = CommandTable {};
    table.onAsync<EchoRequest, EchoResponse>(
        "echo",
        [](const EchoRequest& req, Completer<EchoResponse> done)
        {
            std::thread([req, done] { done.resolve({.echoed = req.text + "!"}); })
                .detach();
        });

    auto settled = std::promise<JSON> {};
    auto future = settled.get_future();
    auto resolve = Resolve {[&settled](const JSON& result, const std::string*)
                            { settled.set_value(result); }};

    table.dispatchAsync("echo", Json::parse(R"({"text":"hi"})"), resolve);

    auto result = future.get();
    check(result["echoed"].asString() == "hi!");
};

auto asyncReject = test("CommandTable onAsync surfaces reject as error") = []
{
    auto table = CommandTable {};
    table.onAsync<EchoRequest, EchoResponse>(
        "echo",
        [](const EchoRequest&, Completer<EchoResponse> done)
        { done.reject("nope"); });

    auto outcome = AsyncOutcome {};
    table.dispatchAsync(
        "echo", Json::parse(R"({"text":"hi"})"), recordInto(outcome));

    check(outcome.hasError);
    check(outcome.error == "nope");
};

auto asyncSingleShot = test("CommandTable onAsync is single-shot, first wins") = []
{
    auto table = CommandTable {};
    table.onAsync<EchoRequest, EchoResponse>(
        "echo",
        [](const EchoRequest&, Completer<EchoResponse> done)
        {
            done.resolve({.echoed = "first"});
            done.reject("second");
            done.resolve({.echoed = "third"});
        });

    auto outcome = AsyncOutcome {};
    table.dispatchAsync("echo", {}, recordInto(outcome));

    check(outcome.settleCount == 1);
    check(!outcome.hasError);
    check(outcome.result["echoed"].asString() == "first");
};

auto asyncDropAutoRejects =
    test("CommandTable onAsync auto-rejects a dropped completer") = []
{
    auto table = CommandTable {};
    table.onAsync<EchoRequest, EchoResponse>(
        "echo", [](const EchoRequest&, Completer<EchoResponse>) {});

    auto outcome = AsyncOutcome {};
    table.dispatchAsync("echo", {}, recordInto(outcome));

    check(outcome.settleCount == 1);
    check(outcome.hasError);
};

auto asyncHas = test("CommandTable::has reflects async registration") = []
{
    auto table = CommandTable {};
    check(!table.has("echo"));

    table.onAsync<EchoRequest, EchoResponse>(
        "echo",
        [](const EchoRequest& req, Completer<EchoResponse> done)
        { done.resolve({.echoed = req.text}); });

    check(table.has("echo"));
};

auto asyncSyncDispatchThrows =
    test("CommandTable::dispatch throws for an async command") = []
{
    auto table = CommandTable {};
    table.onAsync<EchoRequest, EchoResponse>(
        "echo",
        [](const EchoRequest& req, Completer<EchoResponse> done)
        { done.resolve({.echoed = req.text}); });

    auto threw = false;

    try
    {
        table.dispatch("echo", {});
    }
    catch (const std::exception&)
    {
        threw = true;
    }

    check(threw);
};
