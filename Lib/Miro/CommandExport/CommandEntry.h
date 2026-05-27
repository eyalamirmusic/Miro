#pragma once

#include "../Reflection/CommandTable.h"

#include <string>

namespace Miro::CommandExport
{

// One registered command. Carries the C++-derived type identities for
// the request and response (so the export runner can match them up
// against TypeNode trees) plus a JSON-in / JSON-out thunk for runtime
// dispatch.
//
// hasRequest/hasResponse are false when the C++ handler omits the
// corresponding side (zero-arg function / void-returning function).
// The matching type-name fields are empty in that case, and the
// generator emits `()` / `Promise<void>` respectively.
//
// Populated by the codegen path's DescribeReflector walk
// (TypeExport::toCommandEntries). The thunk slot is empty in that path
// — describe mode records metadata only, it doesn't dispatch.
struct CommandEntry
{
    std::string name;

    bool hasRequest = true;
    std::string requestTypeName;
    std::string requestQualifiedName;

    bool hasResponse = true;
    std::string responseTypeName;
    std::string responseQualifiedName;

    CommandTable::RawHandler thunk;
};

} // namespace Miro::CommandExport