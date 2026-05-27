#pragma once

// Bundle of everything a Format functor needs to render. Decouples
// the format pipeline from how the data was sourced — the same
// formats run against:
//   - the static-init process-wide registries (existing main.cpp)
//   - a DescribeReflector walk over one or more API classes
//     (codegenMain<Apis...> — the inversion-of-control path)
//
// typeRoots is taken mutably because rendering may rewrite per-node
// typeName during the disambiguation pass (matches the existing
// formatZodModule / formatTypesModule contract).

#include "../CommandExport/CommandEntry.h"
#include "../JSON/Json.h"
#include "../TypeTree/TypeTree.h"

#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace Miro::TypeExport
{

// One push-event a Format may want to emit a TS binding for. Carries
// the wire name, payload type identity, a factory for the default
// payload JSON (used by hook codegen as the initial value), and the
// optional keyed-collection metadata used by React-style "useFoo(id)"
// hook codegen.
//
// Source-agnostic: populated either by the static-init macros (eacp's
// EACP_EVENT / EACP_STATE / EACP_KEYED_STATE write into the process-
// wide eventRegistry) or by the inversion path's DescribeReflector
// walk (codegenMain<Apis...> translates events into the Context). The
// receiving Format consumes Miro::EventInfo without caring which side
// produced it.
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
    std::span<TypeTree::TypeNode> typeRoots;
    std::span<const CommandExport::CommandEntry> commands;
    std::span<const EventInfo> events;
    std::string_view baseName;
};

} // namespace Miro::TypeExport
