// Demonstration: declare a few reflectable C++ structs, print the
// generated Zod and plain-TS modules to stdout, and drop them as
// User.zod.ts / User.ts next to the executable so they can be imported
// directly from a TS project.

#include <Miro/Miro.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct Address
{
    std::string street;
    std::string zip;

    MIRO_REFLECT(street, zip)
};

enum class Role
{
    Admin,
    Editor,
    Viewer
};

struct User
{
    std::string name;
    int age = 0;
    bool active = true;
    Role role = Role::Viewer;
    Address address;
    std::vector<std::string> tags;
    std::map<std::string, int> counters;
    std::optional<std::string> note;
    std::optional<Address> shippingAddress;

    MIRO_REFLECT(
        name, age, active, role, address, tags, counters, note, shippingAddress)
};

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
