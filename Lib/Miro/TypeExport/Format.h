#pragma once

#include "Context.h"

#include <functional>
#include <string>

// Format extension hook for the Miro codegen runner.
//
// The runner discovers formats at static-init time: built-in formats
// (zod, ts, backend, ts-server, bridge, jsonschema, cpp, cpp-miro,
// cpp-client) register themselves from inside the Miro library;
// downstream libraries plug in additional formats from their own
// translation units the same way.
//
// Adding a format:
//   namespace
//   {
//   [[maybe_unused]] const auto _ = Miro::TypeExport::registerFormat({
//       "hooks",
//       ".hooks.ts",
//       [](const Context& ctx) -> std::string { ... },
//   });
//   }
//
// The registering TU must be linked into the codegen executable —
// either by being part of MiroFormats (Miro's own set), or by being
// placed in an OBJECT library that's spliced into the executable
// alongside MiroFormats and MiroCodegenWriter (see
// CMake/MiroTypeExport.cmake).
namespace Miro::TypeExport
{

struct Format
{
    std::string name; // CLI selector, e.g. "hooks"
    std::string extension; // appended to baseName, e.g. ".hooks.ts"

    // Takes a Context bundling pre-built TypeNodes, registered commands,
    // and the output basename.
    std::function<std::string(const Context& ctx)> generate;
};

namespace Detail
{

// Process-wide registry, populated by static initializers in the
// format-defining TUs. The runner walks this once per invocation.
Vector<Format>& formatRegistry();

} // namespace Detail

// Adds a Format to the process-wide registry. Returns 0 so the call
// can sit in a static-init expression without needing a struct wrapper.
int registerFormat(Format format);

} // namespace Miro::TypeExport
