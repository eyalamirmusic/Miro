#include <Miro/Json.h>

#include <iostream>

int main()
{
    auto json = std::string_view {R"({
        "name": "Miro",
        "version": 1.0,
        "features": ["json", "reflection"],
        "active": true,
        "metadata": null
    })"};

    auto value = Miro::Json::Parser::parse(json);

    std::cout << "name: " << value["name"].asString() << "\n";
    std::cout << "version: " << value["version"].asNumber() << "\n";
    std::cout << "first feature: " << value["features"][std::size_t {0}].asString()
              << "\n";
    std::cout << "active: " << std::boolalpha << value["active"].asBool() << "\n";
    std::cout << "metadata is null: " << std::boolalpha << value["metadata"].isNull()
              << "\n";

    return 0;
}
