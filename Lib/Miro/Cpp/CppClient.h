#pragma once

#include "../CommandExport/CommandEntry.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>
#include <string_view>

namespace Miro::Cpp
{

// Dotted command names ("api.ping") flatten into method names with '_'
// (Client::api_ping); the dotted form is what still goes on the wire.
// typesHeader is a basename — the emitted header includes
// "<typesHeader>.miro.h" by relative path.
std::string formatClientHeader(std::span<TypeTree::TypeNode> typeRoots,
                               std::span<const CommandExport::CommandEntry> commands,
                               std::string_view typesHeader);

} // namespace Miro::Cpp
