#pragma once

#include "../CommandExport/Register.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>
#include <string_view>

namespace Miro::Cpp
{

// Emits a typed C++ client header that mirrors the TypeScript "backend"
// module: one method per registered command, taking and returning the
// reflected request/response types directly. The class is wrapped
// around a transport-agnostic Invoke callable
// (Json::Value(const std::string&, const Json::Value&)), so the same
// generated client works over HTTP, a webview bridge, or any other
// JSON-in / JSON-out transport.
//
// Nested command names (e.g. "api::ping") flatten into method names
// using '_' as the separator (Client::api_ping). The original "::"
// form is preserved on the wire.
//
// `typesHeader` is the basename of the matching .miro.h types file
// (e.g. "schema") emitted by the cpp-miro format. The generated header
// `#include`s "<typesHeader>.miro.h" by relative path.
std::string formatClientHeader(std::span<TypeTree::TypeNode> typeRoots,
                               std::span<const CommandExport::CommandEntry> commands,
                               std::string_view typesHeader);

} // namespace Miro::Cpp
