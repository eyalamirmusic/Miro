#include "JsonReflector.h"

#include <concepts>
#include <type_traits>

namespace Miro
{
namespace
{

ValueKind kindOf(const Json::Value& value)
{
    if (value.isNull())
        return ValueKind::Null;
    if (value.isBool())
        return ValueKind::Bool;
    if (value.isNumber())
        return ValueKind::Number;
    if (value.isString())
        return ValueKind::String;
    if (value.isArray())
        return ValueKind::Array;
    if (value.isObject())
        return ValueKind::Object;

    return ValueKind::Absent;
}

} // namespace

JsonReflector::JsonReflector(Json::Value& slotToUse, Mode modeToUse)
    : slot(slotToUse)
    , mode(modeToUse)
{
}

bool JsonReflector::isSaving() const
{
    return mode == Mode::Save;
}

bool JsonReflector::isLoading() const
{
    return mode == Mode::Load;
}

bool JsonReflector::isSchema() const
{
    return false;
}

ValueKind JsonReflector::kind() const
{
    return kindOf(slot);
}

void JsonReflector::writeNull()
{
    slot = Json::Value {nullptr};
}

void JsonReflector::visit(PrimitiveRef ref)
{
    if (isSaving())
    {
        std::visit(
            [this](auto* ptr)
            {
                using T = std::remove_pointer_t<decltype(ptr)>;
                if constexpr (std::same_as<T, bool> || std::same_as<T, std::string>
                              || std::same_as<T, double>)
                    slot = Json::Value {*ptr};
                else
                    slot = Json::Value {static_cast<double>(*ptr)};
            },
            ref.data);
        return;
    }

    std::visit(
        [this](auto* ptr)
        {
            using T = std::remove_pointer_t<decltype(ptr)>;
            if constexpr (std::same_as<T, bool>)
            {
                if (slot.isBool())
                    *ptr = slot.asBool();
            }
            else if constexpr (std::same_as<T, std::string>)
            {
                if (slot.isString())
                    *ptr = slot.asString();
            }
            else
            {
                if (slot.isNumber())
                    *ptr = static_cast<T>(slot.asNumber());
            }
        },
        ref.data);
}

void JsonReflector::asObject(ScopeBody body)
{
    if (isSaving() && !slot.isObject())
        slot = Json::Value {Json::Object {}};

    body(*this);
}

void JsonReflector::asArray(ScopeBody body)
{
    if (isSaving())
        slot.data.emplace<Json::Array>();

    body(*this);
}

void JsonReflector::asMap(ScopeBody body)
{
    asObject(std::move(body));
}

void JsonReflector::atKey(std::string_view key, ScopeBody body)
{
    if (isSaving())
    {
        if (!slot.isObject())
            slot = Json::Value {Json::Object {}};
        auto& childSlot = slot.asObject()[std::string {key}];
        auto child = JsonReflector {childSlot, mode};
        body(child);
        return;
    }

    if (!slot.isObject())
        return;

    auto& obj = slot.asObject();
    auto it = obj.find(std::string {key});
    if (it == obj.end())
        return;

    auto child = JsonReflector {it->second, mode};
    body(child);
}

void JsonReflector::atIndex(std::size_t index, ScopeBody body)
{
    if (isSaving())
    {
        if (!slot.isArray())
            slot.data.emplace<Json::Array>();
        auto& arr = std::get<Json::Array>(slot.data);
        if (arr.size() <= index)
            arr.resize(index + 1);
        auto child = JsonReflector {arr[index], mode};
        body(child);
        return;
    }

    if (!slot.isArray())
        return;

    auto& arr = std::get<Json::Array>(slot.data);
    if (index >= arr.size())
        return;

    auto child = JsonReflector {arr[index], mode};
    body(child);
}

std::size_t JsonReflector::arraySize() const
{
    return slot.isArray() ? slot.asArray().size() : 0;
}

void JsonReflector::resizeArray(std::size_t newSize)
{
    if (!slot.isArray())
        slot.data.emplace<Json::Array>();
    auto& arr = std::get<Json::Array>(slot.data);
    arr.resize(newSize);
}

std::vector<std::string> JsonReflector::mapKeys() const
{
    auto keys = std::vector<std::string> {};

    if (!slot.isObject())
        return keys;

    for (auto& [key, _]: slot.asObject())
        keys.push_back(key);

    return keys;
}

} // namespace Miro
