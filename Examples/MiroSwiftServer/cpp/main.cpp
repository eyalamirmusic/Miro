// C++ driver for the C++ -> Swift direction.
//
// Builds the generated C++ client (MiroClient::Client) over an invoker that
// crosses into Swift: makeRemoteInvoker wraps the Swift-exported
// miro_swift_dispatch + a per-app handler context (calc_make_handlers).
// Every client call here goes C++ -> C ABI -> Swift dispatch -> a Swift
// handler and back, decoding the JSON response into the generated C++ types.

#include "CalcHandlersC.h"

#include <Miro/Swift/RemoteBridge.h>
#include <Miro/Swift/SwiftServerC.h>

#include "Schema.client.h" // generated cpp-client (includes Schema.miro.h)

#include <iostream>
#include <string>

namespace
{

int failures = 0;

void check(bool condition, const std::string& message)
{
    std::cout << (condition ? "OK   " : "FAIL ") << message << "\n";
    if (!condition)
        ++failures;
}

} // namespace

int main()
{
    auto* ctx = calc_make_handlers();

    auto invoker = Miro::Swift::makeRemoteInvoker(
        &miro_swift_dispatch, ctx, &miro_swift_string_free);
    auto client = MiroClient::Client {invoker};

    auto addReq = AddRequest {};
    addReq.a = 2;
    addReq.b = 3;
    auto sum = client.add(addReq);
    check(sum.result == 5,
          "add(2, 3).result == 5 (got " + std::to_string(sum.result) + ")");

    auto greetReq = GreetRequest {};
    greetReq.name = "Ada";
    auto greeting = client.greet(greetReq);
    check(greeting.message == "Hello, Ada!",
          "greet(\"Ada\") -> " + greeting.message);

    auto status = client.status();
    check(status.ok && status.version == "1.0.0",
          "status() -> ok=" + std::to_string(status.ok)
              + ", version=" + status.version);

    client.reset(); // void(): must not throw
    check(true, "reset() completed without throwing");

    // Error path: an unknown command should surface as a thrown exception.
    try
    {
        auto bogus = invoker("nope", Miro::JSON {Miro::Json::Object {}});
        check(false, "unknown command should have thrown");
    }
    catch (const std::exception& error)
    {
        check(std::string {error.what()}.find("nope") != std::string::npos,
              std::string {"unknown command throws: "} + error.what());
    }

    calc_destroy_handlers(ctx);

    if (failures == 0)
        std::cout << "\nAll integration checks passed.\n";
    else
        std::cout << "\n" << failures << " check(s) failed.\n";

    return failures == 0 ? 0 : 1;
}
