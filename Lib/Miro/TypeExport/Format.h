#pragma once

#include "Context.h"

#include <functional>
#include <string>

// Formats register at static-init time, so the registering TU must be linked
// into the codegen executable in full (see CMake/MiroTypeExport.cmake).
namespace Miro::TypeExport
{

struct Format
{
    std::string name; // CLI selector, e.g. "hooks"
    std::string extension; // appended to baseName, e.g. ".hooks.ts"

    std::function<std::string(const Context& ctx)> generate;
};

namespace Detail
{

Vector<Format>& formatRegistry();

} // namespace Detail

// Returns 0 so the call can sit in a static-init expression.
int registerFormat(Format format);

} // namespace Miro::TypeExport
