#pragma once

#include "../TypeTree/TypeTree.h"
#include "EventRegister.h"
#include "Register.h"

#include <span>
#include <string>
#include <string_view>

namespace Miro::CommandExport
{

// Emits a transport-agnostic TypeScript module exposing each
// registered command as a typed wrapper around an Invoke callback.
// Commands whose names contain "::" become nested objects on the
// returned shape (so api::ping → backend.api.ping). Empty-request
// types and void-returning handlers collapse to no-arg / void
// signatures respectively.
//
// typeRoots must be the TypeNode list for every type referenced by
// the commands — the caller is responsible for building them via
// TypeTree::buildTree<T> or equivalent. typeRoots is mutated in
// place (collision-rewrites typeName), matching the convention of
// the other Miro::TypeScript formatters.
//
// baseName is the stem of the matching schema.ts module (used to
// construct the `import type * as T from './<baseName>'` line).
std::string formatBackendModule(std::span<TypeTree::TypeNode> typeRoots,
                                std::span<const CommandEntry> commands,
                                std::string_view baseName);

// Emits a TypeScript server-side dispatch module symmetrical to the
// C++ `useStaticRegistry()` story: a `Handlers` interface with one
// method per registered command (typed Req in / Res out, sync or
// async), plus an `async dispatch(handlers, command, payload)` switch
// keyed on the wire command name. Server transports (Node HTTP, etc.)
// stay hand-written; this module only generates the typed dispatch
// core. typeRoots and baseName behave identically to formatBackendModule.
std::string formatServerHandlersModule(std::span<TypeTree::TypeNode> typeRoots,
                                       std::span<const CommandEntry> commands,
                                       std::string_view baseName);

// Emits a TypeScript module declaring the `ServerEvents` interface, a
// map from registered event name to payload type. Pair with the
// `bridge` runtime: when the transport is typed as
// `Transport<ServerEvents>`, `backend.on('name', handler)` enforces
// that 'name' is a declared event and infers the handler's payload.
//
// Always emits the `ServerEvents` interface — even when no events have
// been registered — so consuming templates can unconditionally import
// it. An empty registry produces `export interface ServerEvents {}`,
// which intentionally narrows the typed `on` to no events at all.
std::string formatEventsModule(std::span<TypeTree::TypeNode> typeRoots,
                               std::span<const EventEntry> events,
                               std::string_view baseName);

// Emits a TypeScript module of pre-wired React hooks per registered
// event. The emitter picks the helper to use based on what the event
// looks like:
//
//   - Keyed event with matching get command (e.g. `EACP_KEYED_STATE` +
//     `getTodos`) → `makeKeyedStore` + `useXxx` / `useXxxIds` / `useItem`.
//   - Plain event with matching get command (e.g. `EACP_STATE` +
//     `getParameters`) → `makeBridgeStore` + `useXxx`.
//   - Push-only event (`MIRO_EXPORT_EVENT` with no matching getter) →
//     `makeNativeEvent` + `useXxx`.
//
// Imports `backend` from `./backend` and the helpers from `./react`.
// Those are eacp conventions; consumers using a different layout
// either follow the same naming or re-emit this module themselves.
//
// Initial values are pulled from `EventEntry::defaultPayloadJson`,
// which the runtime captures as `toJSON(T{})` — the first render
// always sees the type's default shape, and gets overwritten as soon
// as the get command resolves (or the next push arrives).
std::string formatHooksModule(std::span<TypeTree::TypeNode> typeRoots,
                              std::span<const CommandEntry> commands,
                              std::span<const EventEntry> events,
                              std::string_view baseName);

} // namespace Miro::CommandExport
