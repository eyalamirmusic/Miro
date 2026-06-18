// Tests for the C ABI over Miro::Bridge (Lib/Miro/Swift/BridgeC.h) — the
// transport seam a Swift client's Invoke closure crosses into C++. These
// exercise the JSON-in / JSON-out contract, the error channel, and the
// memory-ownership rules without needing a Swift toolchain.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <Miro/Swift/BridgeC.h>
#include <Miro/Swift/RemoteBridge.h>
#include <NanoTest/NanoTest.h>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

using namespace nano;
using namespace Miro;

namespace
{

struct BCAddRequest
{
    int a = 0;
    int b = 0;

    MIRO_REFLECT(a, b)
};

struct BCAddResponse
{
    int result = 0;

    MIRO_REFLECT(result)
};

struct BCStatusResponse
{
    bool ok = true;

    MIRO_REFLECT(ok)
};

BCAddResponse bcAdd(const BCAddRequest& req)
{
    return {req.a + req.b};
}

BCStatusResponse bcStatus()
{
    return {true};
}

// Binds a couple of handlers and hands back the opaque C handle that is
// really &bridge. The bridge must outlive every dispatch call.
MiroBridge* asHandle(Bridge& bridge)
{
    return reinterpret_cast<MiroBridge*>(&bridge);
}

} // namespace

auto bcDispatchRoundTrip =
    test("BridgeC: dispatch round-trips a request/response command") = []
{
    auto bridge = Bridge {};
    bridge.on("add", &bcAdd);

    auto* error = static_cast<char*>(nullptr);
    auto* result =
        miro_bridge_dispatch(asHandle(bridge), "add", R"({"a":2,"b":3})", &error);

    check(result != nullptr);
    check(error == nullptr);

    auto out = BCAddResponse {};
    fromJSON(out, Json::parse(result));
    check(out.result == 5);

    miro_string_free(result);
};

auto bcEmptyPayload =
    test("BridgeC: null/empty payload is treated as an empty object") = []
{
    auto bridge = Bridge {};
    bridge.on("status", &bcStatus);

    auto* result =
        miro_bridge_dispatch(asHandle(bridge), "status", nullptr, nullptr);
    check(result != nullptr);

    auto out = BCStatusResponse {};
    fromJSON(out, Json::parse(result));
    check(out.ok);

    miro_string_free(result);
};

auto bcUnknownCommand =
    test("BridgeC: unknown command returns null and sets the error") = []
{
    auto bridge = Bridge {};
    bridge.on("add", &bcAdd);

    auto* error = static_cast<char*>(nullptr);
    auto* result = miro_bridge_dispatch(asHandle(bridge), "nope", "{}", &error);

    check(result == nullptr);
    check(error != nullptr);
    check(contains(std::string {error}, "unknown command"));

    miro_string_free(error);
};

auto bcMalformedPayload =
    test("BridgeC: malformed JSON payload surfaces as an error, not a crash") = []
{
    auto bridge = Bridge {};
    bridge.on("add", &bcAdd);

    auto* error = static_cast<char*>(nullptr);
    auto* result =
        miro_bridge_dispatch(asHandle(bridge), "add", "{not json", &error);

    check(result == nullptr);
    check(error != nullptr);

    miro_string_free(error);
};

auto bcFreeNullIsSafe = test("BridgeC: miro_string_free(nullptr) is a no-op") = []
{
    miro_string_free(nullptr);
    check(true);
};

// ---------- RemoteBridge: the reverse direction (C++ caller -> peer) ----------

namespace
{

struct RBMsg
{
    std::string text;

    MIRO_REFLECT(text)
};

// Stand-in for the generated Swift miro_swift_dispatch: echoes the payload
// for "echo", fails (null + *errorOut) for anything else. Allocates with
// malloc so the matching free is plain std::free.
extern "C"
{
    char* rbDispatch(void*, const char* command, const char* payload, char** error)
    {
        if (std::string {command} == "echo")
            return strdup(payload);

        if (error != nullptr)
            *error = strdup((std::string {"no such command: "} + command).c_str());
        return nullptr;
    }

    void rbFree(char* str)
    {
        std::free(str);
    }
}

} // namespace

auto rbInvokerRoundTrip =
    test("RemoteBridge: makeRemoteInvoker round-trips JSON through a raw fn") = []
{
    auto invoker = Swift::makeRemoteInvoker(&rbDispatch, nullptr, &rbFree);

    auto result = invoker("echo", toJSON(RBMsg {"hi"}));

    auto out = RBMsg {};
    fromJSON(out, result);
    check(out.text == "hi");
};

auto rbInvokerError =
    test("RemoteBridge: invoker throws with the peer's message on failure") = []
{
    auto invoker = Swift::makeRemoteInvoker(&rbDispatch, nullptr, &rbFree);

    auto threw = false;
    try
    {
        invoker("boom", JSON {Json::Object {}});
    }
    catch (const std::exception& error)
    {
        threw = true;
        check(contains(std::string {error.what()}, "no such command: boom"));
    }
    check(threw);
};
