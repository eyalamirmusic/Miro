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
    auto opts = Miro::Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = TypeTree::TypeReflector {root, opts};
    auto value = T {};
    Miro::Detail::reflectValue(reflector, value);
}

namespace Detail
{

// Type-erased descriptor of one command declared from a user's
// reflect() body. Carries enough metadata for the describe-mode
// reflector to feed codegen plus a factory closure that, given the
// API-instance pointer, returns a RawHandler ready to install on a
// CommandTable. The pmf itself is captured inside `makeHandler` —
// the descriptor stays POD-like.
struct CommandDescriptor
{
    std::string_view name;

    bool hasReq = false;
    bool hasRes = false;
    std::string_view reqTypeName;
    std::string_view reqQualifiedName;
    std::string_view resTypeName;
    std::string_view resQualifiedName;

    std::function<CommandTable::RawHandler(void* apiInstance)> makeHandler;

    // Walk a structural TypeNode for Req / Res into the supplied node
    // when the corresponding side is present. Empty std::function when
    // the shape elides that side (Res() / void(Req) / void()). The
    // caller controls where the node lives — mirrors the existing
    // buildAllTypeTrees pattern, which constructs roots in place rather
    // than moving them around (TypeNode owns its children via
    // OwningPointer<TypeNode>; safer to never move it after build).
    std::function<void(TypeTree::TypeNode&)> buildReqTree;
    std::function<void(TypeTree::TypeNode&)> buildResTree;
};

// Sibling descriptor for an Event<T> member. `makeListener` builds an
// owning Listener subscribed to the event's broadcaster; the listener
// closes over the supplied `emit` callback, which the binding side
// wires to its bridge's emit channel.
struct EventDescriptor
{
    std::string_view name;
    std::string_view payloadTypeName;
    std::string_view payloadQualifiedName;

    std::function<EA::OwningPointer<EA::Listener>(
        void* apiInstance,
        std::function<void(const JSON& payload)> emit)>
        makeListener;

    // Walks a structural TypeNode for the payload type T (from
    // Event<T>) into the supplied node. Always populated. Same
    // in-place contract as CommandDescriptor::buildReqTree.
    std::function<void(TypeTree::TypeNode&)> buildPayloadTree;

    // Returns toJSON(T{}) — i.e. the wire representation of a
    // default-constructed payload. Hook codegen uses this as the
    // initial value for useFoo() React hooks before the first real
    // emit arrives. Always populated.
    std::function<JSON()> defaultPayloadJson;
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
        d.hasReq = Info::hasReq;
        d.hasRes = Info::hasRes;

        if constexpr (Info::hasReq)
        {
            d.reqTypeName = Miro::Detail::typeNameOf<typename Info::Req>();
            d.reqQualifiedName =
                Miro::Detail::qualifiedNameOf<typename Info::Req>();
            d.buildReqTree = [](TypeTree::TypeNode& root)
            { buildTreeInto<typename Info::Req>(root); };
        }
        if constexpr (Info::hasRes)
        {
            d.resTypeName = Miro::Detail::typeNameOf<typename Info::Res>();
            d.resQualifiedName =
                Miro::Detail::qualifiedNameOf<typename Info::Res>();
            d.buildResTree = [](TypeTree::TypeNode& root)
            { buildTreeInto<typename Info::Res>(root); };
        }

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
        using Info = EventMemberInfo<Pmd>;
        using Payload = typename Info::Payload;

        auto d = Detail::EventDescriptor {};
        d.name = name;
        d.payloadTypeName = Miro::Detail::typeNameOf<Payload>();
        d.payloadQualifiedName = Miro::Detail::qualifiedNameOf<Payload>();
        d.buildPayloadTree = [](TypeTree::TypeNode& root)
        { buildTreeInto<Payload>(root); };
        d.defaultPayloadJson = [] { return toJSON(Payload {}); };

        d.makeListener =
            [member](void* apiInstance,
                     std::function<void(const JSON&)> emit)
            -> EA::OwningPointer<EA::Listener>
        {
            auto& api = *static_cast<typename Info::Class*>(apiInstance);
            auto& event = api.*member;

            return EA::makeOwned<EA::Listener>(
                event.broadcaster(),
                [&event, emit = std::move(emit)]
                { emit(toJSON(event.snapshot())); },
                EA::Listener::Modes::TriggerOnEvent);
        };

        eventImpl(d);
    }

protected:
    virtual void commandImpl(const Detail::CommandDescriptor&) = 0;
    virtual void eventImpl(const Detail::EventDescriptor&) = 0;
};

} // namespace Miro
