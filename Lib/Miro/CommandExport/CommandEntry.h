#pragma once

#include "../Reflection/CommandTable.h"

#include <string>

namespace Miro::CommandExport
{

struct CommandEntry
{
    std::string name;

    bool hasRequest = true;
    std::string requestTypeName;
    std::string requestQualifiedName;

    bool hasResponse = true;
    std::string responseTypeName;
    std::string responseQualifiedName;

    // Empty on the codegen path — describe mode records metadata, never
    // dispatches.
    CommandTable::RawHandler thunk;
};

} // namespace Miro::CommandExport