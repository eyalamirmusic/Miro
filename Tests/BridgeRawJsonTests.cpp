// Its own executable: every TU in MiroTests that sees ReflectJson.h
// instantiates the same toJSON<Miro::JSON>, and the linker may keep
// that one instead of the instantiation compiled here.

#include <Miro/Bridge.h>

#include <NanoTest/NanoTest.h>

#include <string>

using namespace nano;
using namespace Miro;

auto bridgeOnlyEmptyObject =
    test("Bridge-only TU: a top-level empty JSON object saves as {}") = []
{
    auto saved = toJSON(JSON {Json::Object {}});

    check(saved.isObject());
    check(Json::print(saved) == "{}");
};

auto bridgeOnlyAsyncEmptyObject =
    test("Bridge-only TU: an async {} result resolves as {}") = []
{
    auto bridge = Bridge {};
    bridge.onAsync<JSON, JSON>("ping",
                               [](const JSON&, Completer<JSON> done)
                               { done.resolve(JSON {Json::Object {}}); });

    auto result = JSON {};
    bridge.dispatchAsync("ping",
                         JSON {},
                         [&result](const JSON& settled, const std::string*)
                         { result = settled; });

    check(result.isObject());
};
