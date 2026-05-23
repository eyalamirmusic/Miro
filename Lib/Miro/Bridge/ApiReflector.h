#pragma once

// Polymorphic reflector for declaring an API's commands and events.
//
// Mirrors the pattern Miro::Reflector uses for data: the user writes a
// non-static `void reflect(Miro::ApiReflector& r)` method on their API
// class; the library walks that method with a concrete reflector that
// either binds the API to a Bridge at runtime or records its shape for
// codegen.
//
//   class Todos
//   {
//   public:
//       void reflect(Miro::ApiReflector& r)
//       {
//           r.command(&Todos::getTodos,       "getTodos");
//           r.command(&Todos::addTodo,        "addTodo");
//           r.event  (&Todos::changes,        "todos");
//       }
//
//       TodoState getTodos() const;
//       void addTodo(const AddRequest& req);
//
//       Miro::Event<TodoState> changes;
//   };
//
// The templated `command` / `event` entry points sit on this base
// (non-virtual). They use MethodInfo / EventMemberInfo to extract types,
// pack a type-erased descriptor, and dispatch through a virtual hook
// the concrete reflectors override. Same trick CommandTable already
// uses to route typed handlers through a runtime RawHandler.

#include "../Reflection/CommandTable.h"
#include "../Reflection/TypeName.h"
#include "../TypeTree/TypeTree.h"
#include "Callable.h"

#include <ea_data_structures/Pointers/Broadcaster.h>
#include <ea_data_structures/Pointers/OwningPointer.h>

#include <functional>
#include <string_view>
#include <utility>

namespace Miro
{

class Bridge;

// In-place variant of TypeTree::buildTree<T>(). The descriptors carry
// these as `void(TypeNode&)` factories so DescribeReflector can build
// trees directly into an emplace_back'd slot — avoiding any move of a
// TypeNode (which owns its children via OwningPointer<TypeNode> and
// is safer left in place once built).
template <typename T>
void buildTreeInto(TypeTree::TypeNode& root)
{
    auto opts = Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = TypeTree::TypeReflector {root, opts};
    auto value = T {};
    Detail::reflectValue(reflector, value);
}

namespace Detail
{
using NodeFunc = std::function<void(TypeTree::TypeNode&)>;

// One reflected type as seen from a user's reflect() body: the Req
// side of a command, the Res side, or the payload of an Event<T>.
// `buildTree` is empty when the type is elided (e.g. void return,
// nullary command) — operator bool() lets consumers gate cleanly.
struct TypeInfo
{
    std::string_view name;
    std::string_view qualifiedName;

    // In-place TypeNode factory; see buildTreeInto<T> for the contract.
    // Caller controls where the node lives — TypeNode owns its
    // children via OwningPointer<TypeNode> and is safer left in place
    // once built.
    NodeFunc buildTree;

    explicit operator bool() const { return bool(buildTree); }
};

template <typename T>
TypeInfo makeTypeInfo()
{
    return {typeNameOf<T>(),
            qualifiedNameOf<T>(),
            [](TypeTree::TypeNode& root) { buildTreeInto<T>(root); }};
}

// Type-erased descriptor of one command declared from a user's
// reflect() body. Carries enough metadata for the describe-mode
// reflector to feed codegen plus a factory closure that, given the
// API-instance pointer, returns a RawHandler ready to install on a
// CommandTable. The pmf itself is captured inside `makeHandler` —
// the descriptor stays POD-like.
struct CommandDescriptor
{
    using HandlerFunc = std::function<CommandTable::RawHandler(void* apiInstance)>;

    std::string_view name;

    // Empty TypeInfo (operator bool == false) when the shape elides
    // that side: Res() / void(Req) / void().
    TypeInfo req;
    TypeInfo res;

    HandlerFunc makeHandler;
};

// Sibling descriptor for an Event<T> member. `makeListener` builds an
// owning Listener subscribed to the event's broadcaster; the listener
// closes over the supplied `emit` callback, which the binding side
// wires to its bridge's emit channel.
struct EventDescriptor
{
    using PayloadFunc = std::function<void(const JSON& payload)>;
    using ListenerFunc =
        std::function<OwningPointer<EA::Listener>(void*, PayloadFunc)>;

    std::string_view name;

    // Always populated — Event<T> always has a payload type.
    TypeInfo payload;

    ListenerFunc makeListener;

    // Returns toJSON(T{}) — i.e. the wire representation of a
    // default-constructed payload. Hook codegen uses this as the
    // initial value for useFoo() React hooks before the first real
    // emit arrives. Always populated.
    std::function<JSON()> defaultPayloadJson;

    // Keyed-collection metadata, used by React-hook codegen to emit
    // useXxx/useXxxIds/useXxxItem instead of a flat store. Populated
    // only by ApiReflector::keyedEvent — plain event() leaves these
    // empty and downstream formats treat the event as non-keyed.
    bool isKeyed = false;
    std::string_view collectionField;
    std::string_view keyField;
};
} // namespace Detail

class ApiReflector
{
public:
    virtual ~ApiReflector() = default;

    template <typename Pmf>
    void command(Pmf method, std::string_view name)
    {
        using Info = MethodInfo<Pmf>;

        auto d = Detail::CommandDescriptor {};
        d.name = name;

        if constexpr (Info::hasReq)
            d.req = Detail::makeTypeInfo<typename Info::Req>();
        if constexpr (Info::hasRes)
            d.res = Detail::makeTypeInfo<typename Info::Res>();

        d.makeHandler = [method](void* apiInstance) -> CommandTable::RawHandler
        {
            auto& api = *static_cast<typename Info::Class*>(apiInstance);
            return makePmfHandler(method, api);
        };

        commandImpl(d);
    }

    template <typename Pmd>
    void event(Pmd member, std::string_view name)
    {
        eventImpl(makeEventDescriptor(member, name));
    }

    // Same wiring as event(), plus marks the payload as a keyed
    // collection — `collectionField` names a vector-typed field on
    // Payload, `keyField` names the id-like field on each item.
    // React-hook codegen uses this metadata to emit per-id selector
    // hooks (useXxx / useXxxIds / useXxxItem) instead of one flat
    // store. Replaces EACP_KEYED_STATE on the static-init path.
    template <typename Pmd>
    void keyedEvent(Pmd member,
                    std::string_view name,
                    std::string_view collectionField,
                    std::string_view keyField)
    {
        auto d = makeEventDescriptor(member, name);
        d.isKeyed = true;
        d.collectionField = collectionField;
        d.keyField = keyField;
        eventImpl(d);
    }

protected:
    virtual void commandImpl(const Detail::CommandDescriptor&) = 0;
    virtual void eventImpl(const Detail::EventDescriptor&) = 0;

private:
    // Shared construction shared between event() and keyedEvent().
    // Captures Pmd-dependent type info via EventMemberInfo, populates
    // every Pmd-derived field — keyed metadata is filled in by the
    // public keyedEvent() wrapper after this returns.
    template <typename Pmd>
    static Detail::EventDescriptor makeEventDescriptor(Pmd member,
                                                       std::string_view name)
    {
        using Info = EventMemberInfo<Pmd>;
        using Payload = typename Info::Payload;

        auto d = Detail::EventDescriptor {};
        d.name = name;
        d.payload = Detail::makeTypeInfo<Payload>();
        d.defaultPayloadJson = [] { return toJSON(Payload {}); };

        d.makeListener = [member](void* apiInstance,
                                  std::function<void(const JSON&)> emit)
            -> EA::OwningPointer<EA::Listener>
        {
            auto& api = *static_cast<typename Info::Class*>(apiInstance);
            auto& event = api.*member;

            return EA::makeOwned<EA::Listener>(
                event.broadcaster(),
                [&event, emit = std::move(emit)] { emit(toJSON(event.snapshot())); },
                EA::Listener::Modes::TriggerOnEvent);
        };

        return d;
    }
};

} // namespace Miro
