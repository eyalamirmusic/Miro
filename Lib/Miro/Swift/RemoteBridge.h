#pragma once

#include "../JSON/Json.h"

#include <functional>
#include <string>

// C++ side of the C++ -> Swift direction: turn a raw C dispatch function
// (e.g. the generated Swift `miro_swift_dispatch`) plus its context into
// the std::function invoker the generated C++ client
// (Miro::Cpp::formatClientHeader / MiroClient::Client) consumes.
//
// This is the mirror of miro_bridge_dispatch: there C++ is the callee and
// Swift the caller; here C++ is the caller and Swift the callee. The
// generated client stays transport-agnostic — it only sees an Invoke.

namespace Miro::Swift
{

extern "C"
{
    // Raw dispatch the peer exposes: command + JSON-text payload in, owned
    // JSON-text result out (released via FreeStringFn); on failure returns
    // null and sets *errorOut (also released via FreeStringFn).
    using RawDispatchFn = char* (*) (void* ctx,
                                     const char* command,
                                     const char* payloadJson,
                                     char** errorOut);

    using FreeStringFn = void (*)(char* str);
}

// Wraps (dispatchFn, ctx, freeFn) as a JSON-in / JSON-out invoker. Throws
// std::runtime_error carrying the peer's message when dispatch fails.
std::function<Miro::JSON(const std::string& command, const Miro::JSON& payload)>
    makeRemoteInvoker(RawDispatchFn dispatchFn, void* ctx, FreeStringFn freeFn);

} // namespace Miro::Swift
