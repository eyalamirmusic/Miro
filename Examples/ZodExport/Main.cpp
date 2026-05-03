// Manual exporter: prints the Zod / plain-TS modules to stdout and
// writes User.zod.ts / User.ts next to the executable. Uses the same
// Types.h that ZodExportRegistrations registers with the auto-exporter,
// so both stay in sync.

#include "Types.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& contents)
{
    auto out = std::ofstream {path};
    out << contents;
    std::cout << "Wrote " << path.string() << "\n";
}

} // namespace

int main(int /*argc*/, char** argv)
{
    auto zod = Miro::TypeScript::toZod<User>();
    auto types = Miro::TypeScript::toTypes<User>();

    std::cout << zod;
    std::cout << types;

    auto exeDir = std::filesystem::weakly_canonical(std::filesystem::path {argv[0]})
                      .parent_path();

    writeFile(exeDir / "User.zod.ts", zod);
    writeFile(exeDir / "User.ts", types);
    return 0;
}
