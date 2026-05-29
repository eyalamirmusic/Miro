// File-writing half of CodegenMain. Lives in the MiroCodegenWriter
// OBJECT library, not in the Miro static lib, so the ghc::filesystem
// dependency stays scoped to codegen executables and doesn't leak
// into runtime consumers.
//
// The in-memory half (parseCodegenArgs, runFormatsToMemory,
// toCommandEntries) lives in CodegenMain.cpp in Miro proper, so tests
// can drive the pipeline without filesystem access.

#include "CodegenMain.h"
#include "Format.h"

#include <ghc/filesystem.hpp>

#include <fstream>
#include <iostream>
#include <string_view>
#include <system_error>

namespace Miro::TypeExport
{

namespace
{

void writeFile(const ghc::filesystem::path& path, const std::string& contents)
{
    auto out = std::ofstream {path, std::ios::binary};
    out << contents;
    std::cout << "Wrote " << path.string() << "\n";
}

bool isRequested(const Vector<std::string>& requested, std::string_view formatName)
{
    return requested.empty() || requested.contains(std::string {formatName});
}

// Removes files that *would* be emitted by formats we know about but
// that aren't in this run's requested set. Same scope/rationale as
// the original cleanup in Main.cpp — flipping a format off (here or
// via a downstream gate like REACT) drops the corresponding generated
// file on the next build rather than leaving a stale module.
void cleanOrphanedOutputs(const ghc::filesystem::path& outDir,
                          const std::string& baseName,
                          const Vector<std::string>& requested)
{
    for (auto& fmt: Detail::formatRegistry())
    {
        if (isRequested(requested, fmt.name))
            continue;

        auto orphan = outDir / (baseName + fmt.extension);

        std::error_code ec {};
        if (ghc::filesystem::remove(orphan, ec))
            std::cout << "Removed orphan " << orphan.string() << "\n";
    }
}

} // namespace

void writeEmittedFiles(const std::string& outDir,
                       const std::string& baseName,
                       const Vector<EmittedFile>& files,
                       const Vector<std::string>& requestedFormats)
{
    auto outPath = ghc::filesystem::path {outDir};
    ghc::filesystem::create_directories(outPath);

    for (auto& f: files)
        writeFile(outPath / f.filename, f.contents);

    cleanOrphanedOutputs(outPath, baseName, requestedFormats);
}

} // namespace Miro::TypeExport
