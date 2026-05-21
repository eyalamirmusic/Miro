#pragma once

#include "../TypeTree/TypeTree.h"
#include "Register.h"

#include <functional>
#include <string>
#include <string_view>

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
//       [](const auto& entries, std::string_view baseName) -> std::string
//       { ... },
//   });
//   }
//
// The registering TU must be linked into the codegen executable —
// either by being part of Miro itself, or by being placed in an
// OBJECT library that's spliced into the executable alongside
// MiroTypeExportMain (see CMake/MiroTypeExport.cmake).
namespace Miro::TypeExport
{

using EntryList = OwnedVector<TypeEntry>;

struct Format
{
    std::string name;       // CLI selector, e.g. "hooks"
    std::string extension;  // appended to baseName, e.g. ".hooks.ts"

    // baseName is the output filename stem (e.g. "schema"). Formats
    // that emit a self-contained module ignore it; the backend wrapper
    // uses it to import the matching types module by relative path.
    std::function<std::string(const EntryList& entries,
                              std::string_view baseName)>
        generate;
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

// Shared helper most formats need: reflect every registered type into
// its own TypeNode tree. Exposed here so downstream formats don't have
// to reach into the reflection internals.
Vector<TypeTree::TypeNode> buildAllTypeTrees(const EntryList& entries);

} // namespace Miro::TypeExport
