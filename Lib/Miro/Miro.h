#pragma once

#include "Json.h"

#include <memory>
#include <string>
#include <string_view>

namespace Miro
{

struct Reflector
{
    virtual ~Reflector() = default;

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

    Property operator[](std::string_view key) { return {*this, key}; }

    virtual bool isSaving() const = 0;

    virtual void reflect(std::string_view key, Json::Value& value) = 0;

    virtual Child addChild(std::string_view key) = 0;
};

inline void reflect(Reflector& ref, std::string_view key, bool& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = json.asBool();
}

inline void reflect(Reflector& ref, std::string_view key, int& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = static_cast<int>(json.asNumber());
}

inline void reflect(Reflector& ref, std::string_view key, double& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = json.asNumber();
}

inline void reflect(Reflector& ref, std::string_view key, std::string& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = json.asString();
}

template <typename T>
void reflect(Reflector& ref, std::string_view key, T& value)
{
    auto child = ref.addChild(key);
    value.reflect(child);
}

template <typename T>
void Reflector::Property::operator()(T& value)
{
    Miro::reflect(reflector, key, value);
}

} // namespace Miro
