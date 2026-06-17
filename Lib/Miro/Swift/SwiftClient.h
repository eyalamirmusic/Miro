#pragma once

#include "../CommandExport/CommandEntry.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

// Emits a Swift `Client` that wraps an injected `invoke` closure: one
// method per registered command that encodes the request to JSON, calls
// invoke(command, payloadData), and decodes the response. This is the
// Swift -> C++ direction — the C++ Bridge is the callee (host), this
// generated client is the caller.
//
// Mirrors the C++ client (Miro::Cpp::formatClientHeader): dotted command
// names flatten to `a_b_name` methods keyed on the original wire name,
// empty-request types collapse to no-arg methods, and void-returning
// commands drop the return value.
//
// The transport crossing into C++ is hand-written (a thin C-ABI /
// Swift-C++ interop shim forwarding JSON bytes to Bridge::dispatch) —
// Miro generates the typed wrappers, not the wire. No baseName is needed
// because Swift files in one module share scope; the client references
// the generated types directly.
//
// typeRoots is mutated in place (collision-rewrites typeName), matching
// the other Miro export formatters.

namespace Miro::Swift
{

std::string formatClient(std::span<TypeTree::TypeNode> typeRoots,
                         std::span<const CommandExport::CommandEntry> commands);

} // namespace Miro::Swift
