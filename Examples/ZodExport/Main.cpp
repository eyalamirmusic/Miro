// Demonstration: declare a few reflectable C++ structs and print the
// generated Zod TypeScript module to stdout. Pipe to a .ts file to feed
// it into a real TS project.

#include <Miro/Miro.h>

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

struct User
{
    std::string name;
    int age = 0;
    bool active = true;
    Address address;
    std::vector<std::string> tags;
    std::map<std::string, int> counters;
    std::optional<std::string> note;
    std::optional<Address> shippingAddress;

    MIRO_REFLECT(name, age, active, address, tags, counters, note, shippingAddress)
};

int main()
{
    std::cout << "// === Zod schema ===\n\n";
    std::cout << Miro::TypeScript::toZod<User>();
    std::cout << "\n// === Plain TypeScript types ===\n\n";
    std::cout << Miro::TypeScript::toTypes<User>();
    return 0;
}
