#pragma once

/** @file
    TypeScript emitters over a TypeTree (namespace Miro::TypeScript).

    toZod<T>() and toTypes<T>() give a Zod schema module or plain type
    declarations for one reflected type. The format*Module() overloads
    take prepared trees, and the span overloads bundle many roots into one
    module. formatBridgeRuntime() and formatEventsModule() emit the client
    glue for a Bridge; the typed command wrappers come from
    CommandExport::formatBackendModule().

    Every formatter may rename `typeName` on the trees it is given to
    resolve name collisions; pass a copy to keep the originals. Codegen
    executables normally reach all of this through
    TypeExport::codegenMain() and its "zod", "ts", "bridge" and "events"
    formats rather than calling it directly.
*/

#include "../TypeExport/Context.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>
#include <string_view>

namespace Miro::TypeScript
{

/// The format functions take their roots by mutable reference because
/// emission may rewrite per-node `typeName` to disambiguate types from
/// different namespaces that share an unqualified name. Callers that
/// don't want their trees touched should hand over a copy.
std::string formatZodModule(TypeTree::TypeNode& root);
std::string formatTypesModule(TypeTree::TypeNode& root);

/// Multi-root variants — emit one self-contained module that declares
/// every named (object or enum) type reachable from any of the roots,
/// deduped by qualified name. Used by the type-export runner to bundle
/// all registered types into a single .zod.ts / .ts file. The single-
/// root versions above add a default export for anonymous roots; the
/// bundled versions skip that because a module only allows one default
/// export.
std::string formatZodModule(std::span<TypeTree::TypeNode> roots);
std::string formatTypesModule(std::span<TypeTree::TypeNode> roots);

/// Static, schema-independent runtime emitted as the `bridge` format —
/// the transport-agnostic glue (Transport interface, makeBridge factory)
/// that command factories from `<baseName>.backend` bind to. Pair with a
/// transport adapter (e.g. eacp's webViewTransport, an HTTP fetch
/// transport, a WebSocket transport) to get a typed client.
std::string formatBridgeRuntime();

/// Emits a typed event-subscription module: an `Events` interface
/// mapping each wire event name to its payload type, plus an `EventBus`
/// interface whose `subscribe<K extends EventName>` method auto-types
/// the handler payload. Sub-API events arrive here under the same
/// dotted prefix the DescribeReflector recorded (e.g. "files.changed").
/// typeRoots and baseName behave identically to formatBackendModule.
std::string formatEventsModule(std::span<TypeTree::TypeNode> typeRoots,
                               std::span<const TypeExport::EventInfo> events,
                               std::string_view baseName);

/// Zod schema module for T, from a fresh TypeTree.
template <typename T>
std::string toZod()
{
    auto tree = TypeTree::buildTree<T>();
    return formatZodModule(tree);
}

/// TypeScript type declarations for T, from a fresh TypeTree.
template <typename T>
std::string toTypes()
{
    auto tree = TypeTree::buildTree<T>();
    return formatTypesModule(tree);
}

} // namespace Miro::TypeScript
