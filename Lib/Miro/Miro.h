#pragma once

#include "Json.h"

#include <memory>
#include <string>
#include <string_view>

namespace Miro
{
struct Reflector;

struct Property
{
    template <typename T>
    void operator()(T& value);

    Reflector& reflector;
    std::string_view key;
};

struct Child
{
    std::unique_ptr<Reflector> ref;

    operator Reflector&() { return *ref; }
};

struct Reflector
{
    virtual ~Reflector() = default;

    Property operator[](std::string_view key) { return {*this, key}; }

    virtual bool isSaving() const = 0;

    virtual void reflect(std::string_view key, Json::Value& value) = 0;

    virtual Child addChild(std::string_view key) = 0;
};

void reflect(Reflector& ref, std::string_view key, bool& value);
void reflect(Reflector& ref, std::string_view key, int& value);
void reflect(Reflector& ref, std::string_view key, double& value);

void reflect(Reflector& ref, std::string_view key, std::string& value);

template <typename T>
void reflect(Reflector& ref, std::string_view key, T& value)
{
    auto child = ref.addChild(key);
    value.reflect(child);
}

template <typename T>
void Property::operator()(T& value)
{
    Miro::reflect(reflector, key, value);
}

} // namespace Miro
