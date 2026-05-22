#include "CodegenMain.h"

#include "Format.h"

#include <iostream>
#include <string_view>

namespace Miro::TypeExport
{

namespace
{

void usage(const char* exeName)
{
    std::cerr << "Usage: " << exeName
              << " --out <dir> [--name <basename>] [--format <name>]...\n"
              << "  --out <dir>       Directory to write generated files into\n"
              << "  --name <basename> Output filename stem (default: schema)\n"
              << "  --format <name>   Repeatable; defaults to all known formats\n"
              << "Known formats:";
    for (auto& fmt: Detail::formatRegistry())
        std::cerr << " " << fmt.name;
    std::cerr << "\n";
}

bool isFormatRequested(const Vector<std::string>& requested,
                       std::string_view formatName)
{
    return requested.empty() || requested.contains(std::string {formatName});
}

} // namespace

CodegenArgs parseCodegenArgs(int argc, char** argv)
{
    auto args = CodegenArgs {};

    for (auto i = 1; i < argc; ++i)
    {
        auto arg = std::string_view {argv[i]};

        if (arg == "--out" && i + 1 < argc)
            args.outDir = argv[++i];
        else if (arg == "--name" && i + 1 < argc)
            args.baseName = argv[++i];
        else if (arg == "--format" && i + 1 < argc)
            args.requestedFormats.addIfNotThere(argv[++i]);
        else
        {
            usage(argv[0]);
            return args;
        }
    }

    args.valid = !args.outDir.empty();
    if (!args.valid)
        usage(argv[0]);
    return args;
}

Vector<CommandExport::CommandEntry>
    toCommandEntries(const EA::Vector<Miro::Detail::DescribeReflector::CommandRecord>&
                         records)
{
    auto entries = Vector<CommandExport::CommandEntry> {};
    entries.reserve(records.size());

    for (auto& r: records)
    {
        auto entry = CommandExport::CommandEntry {};
        entry.name = r.name;
        entry.hasRequest = r.hasReq;
        entry.requestTypeName = r.reqTypeName;
        entry.requestQualifiedName = r.reqQualifiedName;
        entry.hasResponse = r.hasRes;
        entry.responseTypeName = r.resTypeName;
        entry.responseQualifiedName = r.resQualifiedName;
        // thunk left empty: codegen path doesn't dispatch.
        entries.add(std::move(entry));
    }

    return entries;
}

Vector<EventInfo>
    toEventInfos(const EA::Vector<Miro::Detail::DescribeReflector::EventRecord>&
                     records)
{
    auto infos = Vector<EventInfo> {};
    infos.reserve(records.size());

    for (auto& r: records)
    {
        auto info = EventInfo {};
        info.name = r.name;
        info.payloadTypeName = r.payloadTypeName;
        info.payloadQualifiedName = r.payloadQualifiedName;
        info.defaultPayloadJson = r.defaultPayloadJson;
        info.isKeyed = r.isKeyed;
        info.collectionField = r.collectionField;
        info.keyField = r.keyField;
        infos.add(std::move(info));
    }

    return infos;
}

Vector<EmittedFile>
    runFormatsToMemory(const Context& ctx,
                       const Vector<std::string>& requestedFormats)
{
    auto out = Vector<EmittedFile> {};

    for (auto& fmt: Detail::formatRegistry())
    {
        if (!isFormatRequested(requestedFormats, fmt.name))
            continue;

        auto file = EmittedFile {};
        file.filename = std::string {ctx.baseName} + fmt.extension;
        file.contents = fmt.generate(ctx);
        out.add(std::move(file));
    }

    return out;
}

} // namespace Miro::TypeExport
