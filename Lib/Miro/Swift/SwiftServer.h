#pragma once

#include "../CommandExport/CommandEntry.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

// Emits the Swift callee half (the `swift-server` format) for the
// C++ -> Swift direction: a `Handlers` protocol (one method per command),
// a `dispatch(handlers, command, payload)` switch that decodes / calls /
// encodes, plus a generated @_cdecl C-ABI adapter (miro_swift_dispatch +
// miro_swift_string_free) and box / registration helpers so a C/C++ caller
// can drive the handlers with near-zero hand-written glue.
//
// Mirrors the TypeScript server module (formatServerHandlersModule).
// Dotted command names flatten to `a_b_name` methods keyed on the original
// wire name, matching the Swift client. Requires the `swift-runtime` output
// (MiroDispatchError) in the same module.
//
// typeRoots is mutated in place (collision-rewrites typeName), matching the
// other Miro export formatters.

namespace Miro::Swift
{

std::string formatServer(std::span<TypeTree::TypeNode> typeRoots,
                         std::span<const CommandExport::CommandEntry> commands);

} // namespace Miro::Swift
