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

template <typename T>
constexpr bool isPrimitive()
{
    return std::is_same_v<T, bool> || std::is_same_v<T, int>
           || std::is_same_v<T, double> || std::is_same_v<T, std::string>;
}

// User-facing overload point 1: primitive types via JSON
template <typename T>
    requires(isPrimitive<T>())
void reflect(JSON& json, T& value, bool isSaving)
{
    if (isSaving)
        json = JSON(value);
    else
        value = static_cast<T>(json);
}

// User-facing overload point 2: compound types
template <typename T>
void reflect(Reflector& ref, T& value)
{
    value.reflect(ref);
}

// Keyed dispatch for JSON-reflectable (primitive) types
template <typename T>
    requires(isPrimitive<T>())
void reflect(Reflector& ref, std::string_view key, T& value)
{
    auto json = JSON {};
    if (ref.isSaving())
        reflect(json, value, true);
    ref.reflect(key, json);
    if (!ref.isSaving())
        reflect(json, value, false);
}

// Keyed dispatch for compound types
template <typename T>
    requires(!isPrimitive<T>())
void reflect(Reflector& ref, std::string_view key, T& value)
{
    auto child = ref.addChild(key);
    reflect(static_cast<Reflector&>(child), value);
}

template <typename T>
void Property::operator()(T& value)
{
    Miro::reflect(reflector, key, value);
}

} // namespace Miro
