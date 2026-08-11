#pragma once

#include "../CommandExport/CommandEntry.h"
#include "../JSON/Json.h"
#include "../TypeTree/TypeTree.h"

#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace Miro::TypeExport
{

struct EventInfo
{
    std::string name;
    std::string payloadTypeName;
    std::string payloadQualifiedName;
    std::function<JSON()> defaultPayloadJson;

    bool isKeyed = false;
    std::string collectionField;
    std::string keyField;
};

struct Context
{
    // Mutable: renderers rewrite typeName during name disambiguation.
    std::span<TypeTree::TypeNode> typeRoots;
    std::span<const CommandExport::CommandEntry> commands;
    std::span<const EventInfo> events;
    std::string_view baseName;
};

} // namespace Miro::TypeExport
