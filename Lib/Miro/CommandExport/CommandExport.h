#pragma once

#include "../TypeTree/TypeTree.h"
#include "CommandEntry.h"

#include <span>
#include <string>
#include <string_view>

namespace Miro::CommandExport
{

// Both emitters mutate typeRoots in place (collision-rewrites typeName), and
// take baseName as the stem of the schema module they `import type * as T`
// from. Dotted command names ("api.ping") nest into sub-objects.
std::string formatBackendModule(std::span<TypeTree::TypeNode> typeRoots,
                                std::span<const CommandEntry> commands,
                                std::string_view baseName);

std::string formatServerHandlersModule(std::span<TypeTree::TypeNode> typeRoots,
                                       std::span<const CommandEntry> commands,
                                       std::string_view baseName);

} // namespace Miro::CommandExport
