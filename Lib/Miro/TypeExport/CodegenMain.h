#pragma once

#include "../Bridge/DescribeReflector.h"
#include "../CommandExport/CommandEntry.h"
#include "Context.h"

#include <ea_data_structures/Structures/Vector.h>

#include <string>
#include <string_view>

namespace Miro::TypeExport
{

// outDir is a string, not a path, to keep ghc::filesystem out of this header.
struct CodegenArgs
{
    std::string outDir;
    std::string baseName = "schema";
    Vector<std::string> requestedFormats;
    bool valid = false;
};

CodegenArgs parseCodegenArgs(int argc, char** argv);

// The entries carry no dispatch thunk — codegen only emits.
Vector<CommandExport::CommandEntry> toCommandEntries(
    const Vector<Miro::Detail::DescribeReflector::CommandRecord>& records);

Vector<EventInfo> toEventInfos(
    const Vector<Miro::Detail::DescribeReflector::EventRecord>& records);

struct EmittedFile
{
    std::string filename;
    std::string contents;
};

Vector<EmittedFile> runFormatsToMemory(const Context& ctx,
                                       const Vector<std::string>& requestedFormats);

// Also deletes stale files of registered formats missing from requestedFormats.
void writeEmittedFiles(const std::string& outDir,
                       const std::string& baseName,
                       const Vector<EmittedFile>& files,
                       const Vector<std::string>& requestedFormats);

namespace Detail
{

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
