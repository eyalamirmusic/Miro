#pragma once

#include "../Reflection/CommandTable.h"
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

// Extracts request and response types from a free-function signature
// `Res(const Req&)`. Both the function-type and function-pointer-type
// forms are supported so callers can write either `decltype(fn)` or
// `decltype(&fn)`.
template <typename F>
struct FnTraits;

template <typename Res, typename Req>
struct FnTraits<Res (*)(const Req&)>
{
    using RequestType = Req;
    using ResponseType = Res;
};

template <typename Res, typename Req>
struct FnTraits<Res(const Req&)>
{
    using RequestType = Req;
    using ResponseType = Res;
};

// Process-wide registry, populated by static initializers from
// MIRO_EXPORT_COMMAND(...) at program start. The export runner walks
// this in main(); applications walk it via registerStaticCommandsInto.
std::vector<CommandEntry>& registry();

template <typename Req, typename Res>
struct CommandRegistrar
{
    CommandRegistrar(const char* nameToUse, Res (*handlerToUse)(const Req&))
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
};

} // namespace Detail

// Walks the static command registry and registers each thunk into the
// supplied CommandTable. Lets a runtime dispatcher (e.g. an RPC bridge)
// pick up everything declared via MIRO_EXPORT_COMMAND in one call.
void registerStaticCommandsInto(CommandTable& table);

} // namespace Miro::CommandExport

#define MIRO_EXPORT_COMMAND_CAT2(a, b) a##b
#define MIRO_EXPORT_COMMAND_CAT(a, b) MIRO_EXPORT_COMMAND_CAT2(a, b)

// Registers a free function as a command exposed across the bridge.
// Use at namespace scope after the handler is fully declared:
//
//   PingResponse handlePing(const EmptyMessage&);
//   MIRO_EXPORT_COMMAND(ping, handlePing)
//
// Side effects:
//   - Registers Req and Res with the type export registry, so they
//     show up in generated schema modules.
//   - Records (name, type identities, JSON thunk) in the command
//     registry, picked up by the export runner and by
//     registerStaticCommandsInto() at runtime.
//
// Uniqueness of the generated identifier comes from __LINE__, matching
// MIRO_EXPORT_TYPES. Two MIRO_EXPORT_COMMAND on the same source line
// would collide, which is unrealistic.
#define MIRO_EXPORT_COMMAND(name, fn)                                               \
    namespace                                                                       \
    {                                                                               \
    using MIRO_EXPORT_COMMAND_CAT(miroCommandTraits_, __LINE__) =                   \
        ::Miro::CommandExport::Detail::FnTraits<decltype(&(fn))>;                   \
    [[maybe_unused]] const ::Miro::CommandExport::Detail::CommandRegistrar<         \
        typename MIRO_EXPORT_COMMAND_CAT(miroCommandTraits_,                        \
                                         __LINE__)::RequestType,                    \
        typename MIRO_EXPORT_COMMAND_CAT(miroCommandTraits_,                        \
                                         __LINE__)::ResponseType>                   \
        MIRO_EXPORT_COMMAND_CAT(miroCommandRegistrar_, __LINE__) {#name, fn};       \
    }
