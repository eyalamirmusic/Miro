#pragma once

#include "../Containers.h"
#include "../Reflection/Serialize.h"
#include "../Reflection/TypeName.h"
#include "../TypeExport/Register.h"

#include <functional>
#include <string>

namespace Miro::CommandExport
{

// One registered bridge event. Captures the wire-format name and the
// payload type identity so codegen can emit a typed `ServerEvents` map
// that lets `backend.on('foo', payload => ...)` infer `payload`'s type.
// Runtime emit() is unaffected — the entry is metadata only.
//
// `defaultPayloadJson` returns toJSON(T{}) — used by codegen to emit a
// sensible TS initial value for hook factories without forcing the
// user to repeat it.
//
// `isKeyed`, `collectionField`, `keyField` mark events whose payload
// is a vector keyed by an id-style field. Set via registerKeyedEvent.
// Codegen resolves the actual item type and key TS type by walking
// the payload's TypeNode tree at emit time, so this struct holds only
// the user-supplied names.
struct EventEntry
{
    std::string name;
    std::string payloadTypeName;
    std::string payloadQualifiedName;
    std::function<JSON()> defaultPayloadJson;

    bool isKeyed = false;
    std::string collectionField;
    std::string keyField;
};

namespace Detail
{

// Process-wide registry, populated by static initializers from
// MIRO_EXPORT_EVENT(...) at program start. The export runner walks
// this in main() to emit schema.events.ts.
Vector<EventEntry>& eventRegistry();

template <typename T>
inline void registerEvent(const char* nameToUse)
{
    using Miro::Detail::qualifiedNameOf;
    using Miro::Detail::typeNameOf;
    using TypeExport::Detail::registerOne;

    registerOne<T>();

    auto entry = EventEntry {};
    entry.name = nameToUse;
    entry.payloadTypeName = typeNameOf<T>();
    entry.payloadQualifiedName = qualifiedNameOf<T>();
    entry.defaultPayloadJson = [] { return Miro::toJSON(T {}); };
    eventRegistry().add(std::move(entry));
}

// Marks the most-recently-registered event as a keyed collection.
// Pairs with registerEvent: call registerEvent<T>(name) first to put
// the entry on the registry, then markEventKeyed to attach the field
// names. Split this way so EACP_KEYED_STATE can layer keying on top of
// the existing event-registration machinery without forking it.
inline void markEventKeyed(const char* collectionField, const char* keyField)
{
    auto& registry = eventRegistry();
    if (registry.size() == 0)
        return;

    auto& entry = registry[registry.size() - 1];
    entry.isKeyed = true;
    entry.collectionField = collectionField;
    entry.keyField = keyField;
}

template <typename T>
inline void registerKeyedEvent(const char* nameToUse,
                               const char* collectionField,
                               const char* keyField)
{
    registerEvent<T>(nameToUse);
    markEventKeyed(collectionField, keyField);
}

} // namespace Detail
} // namespace Miro::CommandExport

#define MIRO_EXPORT_EVENT_CAT2(a, b) a##b
#define MIRO_EXPORT_EVENT_CAT(a, b) MIRO_EXPORT_EVENT_CAT2(a, b)

// Registers a bridge event so codegen can emit a typed handler for it
// on the TS side. Pair with a runtime emit() call (typically inside
// bindToBridge or your own broadcaster wiring); MIRO_EXPORT_EVENT only
// supplies codegen metadata, not runtime behaviour.
//
//   MIRO_EXPORT_EVENT(todos, TodoState)
//
// emits:
//
//   interface ServerEvents { todos: TodoState; }
//
// and lets `backend.on('todos', s => ...)` infer `s` as TodoState.
//
// The payload type is captured via __VA_ARGS__ so it can include
// commas from template arguments without breaking macro expansion
// (e.g. MIRO_EXPORT_EVENT(prices, std::map<std::string, double>)).
#define MIRO_EXPORT_EVENT(name, ...)                                                \
    namespace                                                                       \
    {                                                                               \
    [[maybe_unused]] const auto MIRO_EXPORT_EVENT_CAT(miroEventRegistry_,           \
                                                      __LINE__) = []                \
    {                                                                               \
        ::Miro::CommandExport::Detail::registerEvent<__VA_ARGS__>(#name);           \
        return 0;                                                                   \
    }();                                                                            \
    }
