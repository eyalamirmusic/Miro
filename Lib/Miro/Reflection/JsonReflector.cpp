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

JsonReflector::JsonReflector(Json::Value& slotToUse, Options optsToUse)
    : JsonReflector(slotToUse, optsToUse, false)
{
}

JsonReflector::JsonReflector(Json::Value& slotToUse,
                             Options optsToUse,
                             bool absentToUse)
    : Reflector(optsToUse)
    , slot(slotToUse)
    , absent(absentToUse)
{
    if (isSaving() && !absent)
        commitShape();
}

JsonReflector::~JsonReflector() = default;

void JsonReflector::commitShape()
{
    switch (opts.shape)
    {
        case Shape::Primitive:
            // visit() will write the value directly into the slot.
            break;
        case Shape::Object:
        case Shape::Map:
            if (!slot.isObject())
                slot = Json::Value {Json::Object {}};
            break;
        case Shape::Array:
            if (!slot.isArray())
                slot.data.emplace<Json::Array>();
            break;
    }
}

ValueKind JsonReflector::kind() const
{
    if (absent)
        return ValueKind::Absent;

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

Reflector& JsonReflector::spawnChild(Json::Value& targetSlot,
                                     Options childOpts,
                                     bool absentToUse)
{
    // Destroy the previous child *before* constructing the new one so
    // any subclass with destructor side effects (e.g. emitting close
    // brackets) sees a strict open-then-close ordering.
    currentChild.reset();
    currentChild = std::unique_ptr<JsonReflector>(
        new JsonReflector(targetSlot, childOpts, absentToUse));
    return *currentChild;
}

Reflector& JsonReflector::atKey(std::string_view key, Options childOpts)
{
    if (isSaving())
    {
        if (!slot.isObject())
            slot = Json::Value {Json::Object {}};
        return spawnChild(slot.asObject()[std::string {key}], childOpts, false);
    }

    missingSlot = Json::Value {nullptr};

    if (!slot.isObject())
        return spawnChild(missingSlot, childOpts, true);

    auto& obj = slot.asObject();
    auto it = obj.find(std::string {key});
    if (it == obj.end())
        return spawnChild(missingSlot, childOpts, true);

    return spawnChild(it->second, childOpts, false);
}

Reflector& JsonReflector::atIndex(std::size_t index, Options childOpts)
{
    if (isSaving())
    {
        if (!slot.isArray())
            slot.data.emplace<Json::Array>();
        auto& arr = std::get<Json::Array>(slot.data);
        if (arr.size() <= index)
            arr.resize(index + 1);
        return spawnChild(arr[index], childOpts, false);
    }

    missingSlot = Json::Value {nullptr};

    if (!slot.isArray())
        return spawnChild(missingSlot, childOpts, true);

    auto& arr = std::get<Json::Array>(slot.data);
    if (index >= arr.size())
        return spawnChild(missingSlot, childOpts, true);

    return spawnChild(arr[index], childOpts, false);
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
