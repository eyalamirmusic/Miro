#pragma once

// Templated main() body for codegen executables. Each API class is
// default-constructed, walked via its reflect(ApiReflector&) into a
// local DescribeReflector, and the resulting commands + events +
// TypeNodes feed the Format pipeline.
//
// A consumer's codegen TU is typically two lines:
//
//   #include <Miro/Miro.h>
//   #include "TodoApi.h"
//
//   int main(int argc, char** argv)
//   {
//       return Miro::TypeExport::codegenMain<Api::Todos>(argc, argv);
//   }
//
// The API class's reflect() body is the single source of truth — no
// static-init registries, no WHOLE_ARCHIVE.

#include "../Bridge/DescribeReflector.h"
#include "../CommandExport/CommandEntry.h"
#include "Context.h"

#include <ea_data_structures/Structures/Vector.h>

#include <string>
#include <string_view>

namespace Miro::TypeExport
{

// Parsed --out / --name / --format arguments for a codegen run.
// outDir is a plain string in the public API to keep ghc::filesystem
// out of the umbrella header — the .cpp converts to a filesystem path
// when actually performing I/O.
struct CodegenArgs
{
    std::string outDir;
    std::string baseName = "schema";
    Vector<std::string> requestedFormats;
    bool valid = false;
};

CodegenArgs parseCodegenArgs(int argc, char** argv);

// Translates DescribeReflector's CommandRecord shape (strings only)
// into CommandExport::CommandEntry, the shape the existing Format
// functors consume. The runtime thunk is left empty — describe mode
// doesn't dispatch, only emits.
Vector<CommandExport::CommandEntry> toCommandEntries(
    const Vector<Miro::Detail::DescribeReflector::CommandRecord>& records);

// Translates DescribeReflector's EventRecord shape into the Miro::
// EventInfo entries Context::events carries. Keyed metadata stays
// default (Phase E adds ApiReflector::keyedEvent).
Vector<EventInfo> toEventInfos(
    const Vector<Miro::Detail::DescribeReflector::EventRecord>& records);

// Filename + contents pair for one emitted artifact. runFormatsToMemory
// returns these so tests can inspect output without touching the
// filesystem.
struct EmittedFile
{
    std::string filename;
    std::string contents;
};

// Runs every requested format against ctx, returning the (filename,
// contents) pairs in registry order. The static-init main and
// codegenMain both delegate to this — the only difference between
// them is how ctx was sourced.
Vector<EmittedFile> runFormatsToMemory(const Context& ctx,
                                       const Vector<std::string>& requestedFormats);

// Writes each EmittedFile under outDir; creates the directory if it
// doesn't exist. Removes orphans for formats present in the registry
// but absent from the current run's requested set (same semantics as
// the existing Main.cpp cleanup).
void writeEmittedFiles(const std::string& outDir,
                       const std::string& baseName,
                       const Vector<EmittedFile>& files,
                       const Vector<std::string>& requestedFormats);

namespace Detail
{

// Default-constructs each Api and walks its reflect() into the
// supplied DescribeReflector. Order matches the parameter pack —
// commands and events appear in the order their owning APIs are
// listed at the codegenMain<...> call site.
template <typename... Apis>
void describeAll(Miro::Detail::DescribeReflector& r)
{
    (
        [&]
        {
            auto api = Apis {};
            api.reflect(r);
        }(),
        ...);
}

} // namespace Detail

// In-memory codegen entry point. Walks the listed APIs, then runs the
// format pipeline against the gathered data. Tests use this; the
// templated codegenMain wraps it with arg parsing and file I/O.
template <typename... Apis>
Vector<EmittedFile> buildCodegen(std::string_view baseName,
                                 const Vector<std::string>& requestedFormats)
{
    auto describe = Miro::Detail::DescribeReflector {};
    Detail::describeAll<Apis...>(describe);

    auto commandEntries = toCommandEntries(describe.commands);
    auto eventInfos = toEventInfos(describe.events);

    auto ctx = Context {
        .typeRoots = std::span<TypeTree::TypeNode> {describe.typeRoots.data(),
                                                    static_cast<std::size_t>(
                                                        describe.typeRoots.size())},
        .commands =
            std::span<const CommandExport::CommandEntry> {
                commandEntries.data(),
                static_cast<std::size_t>(commandEntries.size())},
        .events =
            std::span<const EventInfo> {eventInfos.data(),
                                        static_cast<std::size_t>(eventInfos.size())},
        .baseName = baseName,
    };

    return runFormatsToMemory(ctx, requestedFormats);
}

// Full main(): parses args, walks the APIs, runs the format pipeline,
// writes one file per requested format, removes orphans. Returns the
// exit code the executable should propagate.
template <typename... Apis>
int codegenMain(int argc, char** argv)
{
    auto args = parseCodegenArgs(argc, argv);
    if (!args.valid)
        return 1;

    auto files = buildCodegen<Apis...>(args.baseName, args.requestedFormats);
    writeEmittedFiles(args.outDir, args.baseName, files, args.requestedFormats);
    return 0;
}

} // namespace Miro::TypeExport
