#pragma once

#include <Miro/Miro.h>

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
