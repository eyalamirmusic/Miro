#pragma once

#include "../Reflection/CommandTable.h"
#include "../Reflection/ReflectMacro.h"
#include "../Reflection/TypeName.h"
#include "../TypeExport/Register.h"

#include <functional>
#include <string>
#include <vector>

namespace Miro::CommandExport
{

// One registered command. Carries the C++-derived type identities for
// the request and response (so the export runner can match them up
// against TypeNode trees) plus a JSON-in / JSON-out thunk for runtime
// dispatch.
struct CommandEntry
{
    std::string name;

    std::string requestTypeName;
    std::string requestQualifiedName;
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

// Drives one command registration. Templated on a free-function pointer
// `Res(const Req&)` so Req/Res are deduced from the call site — the
// macros just hand over the symbol's address and stringified name.
template <typename Res, typename Req>
inline void registerCommand(const char* nameToUse, Res (*handlerToUse)(const Req&))
{
    using Miro::Detail::qualifiedNameOf;
    using Miro::Detail::typeNameOf;
    using TypeExport::Detail::registerOne;

    registerOne<Req>();
    registerOne<Res>();

    auto entry = CommandEntry {};
    entry.name = nameToUse;
    entry.requestTypeName = typeNameOf<Req>();
    entry.requestQualifiedName = qualifiedNameOf<Req>();
    entry.responseTypeName = typeNameOf<Res>();
    entry.responseQualifiedName = qualifiedNameOf<Res>();
    entry.thunk = CommandTable::createRawHandler<Req, Res>(
        std::function<Res(const Req&)> {handlerToUse});

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
// calls `await backend.<fn>(...)`.
#define MIRO_EXPORT_COMMAND_ITEM(fn)                                                \
    ::Miro::CommandExport::Detail::registerCommand(#fn, &(fn));

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
