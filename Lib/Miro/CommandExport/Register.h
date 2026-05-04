#pragma once

#include "../Reflection/CommandTable.h"
#include "../Reflection/ReflectMacro.h"
#include "../Reflection/Serialize.h"
#include "../Reflection/TypeName.h"
#include "../TypeExport/Register.h"

#include <string>
#include <type_traits>
#include <vector>

namespace Miro::CommandExport
{

// One registered command. Carries the C++-derived type identities for
// the request and response (so the export runner can match them up
// against TypeNode trees) plus a JSON-in / JSON-out thunk for runtime
// dispatch.
//
// hasRequest/hasResponse are false when the C++ handler omits the
// corresponding side (zero-arg function / void-returning function).
// The matching type-name fields are empty in that case, and the
// generator emits `()` / `Promise<void>` respectively.
struct CommandEntry
{
    std::string name;

    bool hasRequest = true;
    std::string requestTypeName;
    std::string requestQualifiedName;

    bool hasResponse = true;
    std::string responseTypeName;
    std::string responseQualifiedName;

    Miro::CommandTable::RawHandler thunk;
};

namespace Detail
{

// Process-wide registry, populated by static initializers from
// MIRO_EXPORT_COMMAND(S)(...) at program start. The export runner walks
// this in main(); applications walk it via registerStaticCommandsInto.
std::vector<CommandEntry>& registry();

// Trait that classifies a free-function handler. Specializations cover
// the four shapes we accept: Res/void return, with/without a const-ref
// request argument. Req/Res are still spelled even when absent (as
// void) so that if-constexpr discarded branches stay well-formed.
template <typename F>
struct CommandSignature;

template <typename R, typename A>
struct CommandSignature<R (*)(const A&)>
{
    using Res = R;
    using Req = A;
    static constexpr bool hasReq = true;
    static constexpr bool hasRes = !std::is_void_v<R>;
};

template <typename R>
struct CommandSignature<R (*)()>
{
    using Res = R;
    using Req = void;
    static constexpr bool hasReq = false;
    static constexpr bool hasRes = !std::is_void_v<R>;
};

// Templated on the handler's address (passed as a non-type template
// parameter) so we can branch on its signature with if constexpr.
// Registers Req/Res with the type-export registry only when present,
// and builds a thunk that adapts the four shapes to the uniform
// JSON-in / JSON-out interface CommandTable expects.
template <auto Handler>
inline void registerCommand(const char* nameToUse)
{
    using Miro::Detail::qualifiedNameOf;
    using Miro::Detail::typeNameOf;
    using TypeExport::Detail::registerOne;

    using Sig = CommandSignature<decltype(Handler)>;

    auto entry = CommandEntry {};
    entry.name = nameToUse;

    if constexpr (Sig::hasReq)
    {
        using Req = Sig::Req;
        registerOne<Req>();
        entry.hasRequest = true;
        entry.requestTypeName = typeNameOf<Req>();
        entry.requestQualifiedName = qualifiedNameOf<Req>();
    }
    else
    {
        entry.hasRequest = false;
    }

    if constexpr (Sig::hasRes)
    {
        using Res = Sig::Res;
        registerOne<Res>();
        entry.hasResponse = true;
        entry.responseTypeName = typeNameOf<Res>();
        entry.responseQualifiedName = qualifiedNameOf<Res>();
    }
    else
    {
        entry.hasResponse = false;
    }

    entry.thunk = [](const Miro::Json::Value& payload) -> Miro::Json::Value
    {
        if constexpr (Sig::hasReq)
        {
            using Req = Sig::Req;
            auto req = Req {};
            auto adjusted = payload.isNull()
                                ? Miro::Json::Value {Miro::Json::Object {}}
                                : payload;
            Miro::fromJSON(req, adjusted);

            if constexpr (Sig::hasRes)
                return Miro::toJSON(Handler(req));
            else
            {
                Handler(req);
                return Miro::Json::Value {};
            }
        }
        else
        {
            if constexpr (Sig::hasRes)
                return Miro::toJSON(Handler());
            else
            {
                Handler();
                return Miro::Json::Value {};
            }
        }
    };

    registry().push_back(std::move(entry));
}

} // namespace Detail

// Walks the static command registry and registers each thunk into the
// supplied CommandTable. Lets a runtime dispatcher (e.g. an RPC bridge)
// pick up everything declared via MIRO_EXPORT_COMMAND(S) in one call.
void registerStaticCommandsInto(CommandTable& table);

} // namespace Miro::CommandExport

#define MIRO_EXPORT_COMMAND_CAT2(a, b) a##b
#define MIRO_EXPORT_COMMAND_CAT(a, b) MIRO_EXPORT_COMMAND_CAT2(a, b)

// Single registration step used inside the variadic fan-out. The
// function's identifier doubles as the command name, so the JS side
// calls `await backend.<fn>(...)`. The handler is passed as a non-type
// template argument, which lets registerCommand branch on the
// signature with if constexpr to support all four shapes:
// Res(Req), Res(), void(Req), void().
#define MIRO_EXPORT_COMMAND_ITEM(fn)                                                \
    ::Miro::CommandExport::Detail::registerCommand<&(fn)>(#fn);

// Registers one or more free functions as commands exposed across the
// bridge. Use at namespace scope after the handler(s) are fully
// declared:
//
//   PingResponse ping(const Miro::EmptyValue&);
//   EchoResponse echo(const EchoRequest&);
//   MIRO_EXPORT_COMMANDS(ping, echo)
//
// Side effects:
//   - Registers Req and Res with the type export registry, so they
//     show up in generated schema modules.
//   - Records (name, type identities, JSON thunk) in the command
//     registry, picked up by the export runner and by
//     registerStaticCommandsInto() at runtime.
//
// All items share one anonymous static-init lambda, so __LINE__ only
// needs to disambiguate at the macro-call level. Two MIRO_EXPORT_
// COMMAND(S) calls on the same source line would collide on the
// generated identifier, which is unrealistic.
#define MIRO_EXPORT_COMMANDS(...)                                                   \
    namespace                                                                       \
    {                                                                               \
    [[maybe_unused]] const auto MIRO_EXPORT_COMMAND_CAT(miroCommandRegistry_,       \
                                                       __LINE__) = []               \
    {                                                                               \
        MIRO_FOR_EACH(MIRO_EXPORT_COMMAND_ITEM, __VA_ARGS__)                        \
        return 0;                                                                   \
    }();                                                                            \
    }

// Single-handler alias for clarity at one-off call sites:
//   MIRO_EXPORT_COMMAND(ping)
#define MIRO_EXPORT_COMMAND(fn) MIRO_EXPORT_COMMANDS(fn)
