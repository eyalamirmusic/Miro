#pragma once

#include "../TypeTree/TypeTree.h"
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

} // namespace Miro::CommandExport
