#include "Miro.h"

namespace Miro
{

void reflect(Reflector& ref, std::string_view key, bool& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = json.asBool();
}

void reflect(Reflector& ref, std::string_view key, int& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = static_cast<int>(json.asNumber());
}

void reflect(Reflector& ref, std::string_view key, double& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = json.asNumber();
}

void reflect(Reflector& ref, std::string_view key, std::string& value)
{
    auto json = Json::Value(value);
    ref.reflect(key, json);

    if (!ref.isSaving())
        value = json.asString();
}
} // namespace Miro
