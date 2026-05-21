// Pre-built main() for the Miro type-export runner. Linked into the
// executable target created by miro_export() in CMake. User code
// provides MIRO_EXPORT_TYPES(...) registrations; this file walks the
// type registry once per requested format and writes one file per
// format.
//
// Formats themselves register into Format.h's process-wide registry at
// static-init time — see BuiltinFormats.cpp for Miro's own set and
// any downstream library's codegen sources for extensions.

#include "Format.h"
#include "Register.h"

#include <ghc/filesystem.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

void writeFile(const ghc::filesystem::path& path, const std::string& contents)
{
    auto out = std::ofstream {path};
    out << contents;
    std::cout << "Wrote " << path.string() << "\n";
}

void usage(const char* exeName)
{
    std::cerr << "Usage: " << exeName
              << " --out <dir> [--name <basename>] [--format <name>]...\n"
              << "  --out <dir>       Directory to write generated files into\n"
              << "  --name <basename> Output filename stem (default: schema)\n"
              << "  --format <name>   Repeatable; defaults to all known formats\n"
              << "Known formats:";
    for (auto& fmt: Miro::TypeExport::Detail::formatRegistry())
        std::cerr << " " << fmt.name;
    std::cerr << "\n";
}

struct Args
{
    ghc::filesystem::path outDir;
    std::string baseName = "schema";
    Miro::Vector<std::string> requestedFormats;
    bool valid = false;
};

Args parseArgs(int argc, char** argv)
{
    auto args = Args {};

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
            return args;
    }

    args.valid = !args.outDir.empty();
    return args;
}

bool isFormatRequested(const Miro::Vector<std::string>& requested,
                       std::string_view formatName)
{
    return requested.empty() || requested.contains(std::string {formatName});
}

} // namespace

int main(int argc, char** argv)
{
    auto args = parseArgs(argc, argv);

    if (!args.valid)
    {
        usage(argv[0]);
        return 1;
    }

    ghc::filesystem::create_directories(args.outDir);

    auto& entries = Miro::TypeExport::Detail::registry();

    if (entries.empty())
    {
        std::cerr << "No types registered. The registration library must be "
                     "linked with WHOLE_ARCHIVE so its static initializers "
                     "survive linking.\n";
        return 1;
    }

    for (auto& fmt: Miro::TypeExport::Detail::formatRegistry())
    {
        if (!isFormatRequested(args.requestedFormats, fmt.name))
            continue;

        auto fileName = args.baseName + fmt.extension;
        writeFile(args.outDir / fileName, fmt.generate(entries, args.baseName));
    }

    return 0;
}
