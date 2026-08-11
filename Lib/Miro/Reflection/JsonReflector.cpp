#include "JsonReflector.h"

#include <concepts>
#include <type_traits>

namespace Miro
{
namespace
{

ValueKind kindOf(const JSON& value)
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

Json::Array& ensureArray(JSON& slot)
{
    if (!slot.isArray())
        slot.data.emplace<Json::Array>();

    return std::get<Json::Array>(slot.data);
}

Json::Object& ensureObject(JSON& slot)
{
    if (!slot.isObject())
        slot = JSON {Json::Object {}};

    return slot.asObject();
}

template <typename T>
void writeSlotFromPrimitive(JSON& slot, T* ptr)
{
    if constexpr (std::same_as<T, bool> || std::same_as<T, std::string>
                  || std::same_as<T, double>)
        slot = JSON {*ptr};
    else
        slot = JSON {static_cast<double>(*ptr)};
}

template <typename T>
void readSlotIntoPrimitive(const JSON& slot, T* ptr)
{
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
}

} // namespace

JsonReflector::JsonReflector(JSON& slotToUse, Options optsToUse)
    : JsonReflector(slotToUse, optsToUse, false)
{
}

JsonReflector::JsonReflector(JSON& slotToUse, Options optsToUse, bool absentToUse)
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
            break;
        case Shape::Object:
        case Shape::Map:
            ensureObject(slot);
            break;
        case Shape::Array:
            ensureArray(slot);
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
    slot = JSON {nullptr};
}

void JsonReflector::visit(PrimitiveRef ref)
{
    if (isSaving())
        savePrimitive(ref);
    else
        loadPrimitive(ref);
}

void JsonReflector::savePrimitive(PrimitiveRef ref)
{
    std::visit([this](auto* ptr) { writeSlotFromPrimitive(slot, ptr); }, ref.data);
}

void JsonReflector::loadPrimitive(PrimitiveRef ref)
{
    std::visit([this](auto* ptr) { readSlotIntoPrimitive(slot, ptr); }, ref.data);
}

Reflector&
    JsonReflector::spawnChild(JSON& targetSlot, Options childOpts, bool absentToUse)
{
    // Reset before constructing so a child with destructor side effects
    // always closes before the next one opens.
    currentChild.reset();
    currentChild = new JsonReflector(targetSlot, childOpts, absentToUse);
    return *currentChild;
}

Reflector& JsonReflector::spawnMissingChild(Options childOpts)
{
    missingSlot = JSON {nullptr};
    return spawnChild(missingSlot, childOpts, true);
}

Reflector& JsonReflector::atKey(std::string_view key, Options childOpts)
{
    if (isSaving())
        return atKeyForSave(key, childOpts);

    return atKeyForLoad(key, childOpts);
}

Reflector& JsonReflector::atKeyForSave(std::string_view key, Options childOpts)
{
    auto& obj = ensureObject(slot);
    return spawnChild(obj[std::string {key}], childOpts, false);
}

Reflector& JsonReflector::atKeyForLoad(std::string_view key, Options childOpts)
{
    if (!slot.isObject())
        return spawnMissingChild(childOpts);

    auto& obj = slot.asObject();
    auto it = obj.find(std::string {key});

    if (it == obj.end())
        return spawnMissingChild(childOpts);

    return spawnChild(it->second, childOpts, false);
}

Reflector& JsonReflector::atIndex(std::size_t index, Options childOpts)
{
    if (isSaving())
        return atIndexForSave(index, childOpts);

    return atIndexForLoad(index, childOpts);
}

Reflector& JsonReflector::atIndexForSave(std::size_t index, Options childOpts)
{
    auto& arr = ensureArray(slot).getVector();

    if (arr.size() <= index)
        arr.resize(index + 1);

    return spawnChild(arr[index], childOpts, false);
}

Reflector& JsonReflector::atIndexForLoad(std::size_t index, Options childOpts)
{
    if (!slot.isArray())
        return spawnMissingChild(childOpts);

    auto& arr = std::get<Json::Array>(slot.data).getVector();

    if (index >= arr.size())
        return spawnMissingChild(childOpts);

    return spawnChild(arr[index], childOpts, false);
}

std::size_t JsonReflector::arraySize() const
{
    return slot.isArray() ? slot.asArray().getVector().size() : 0;
}

void JsonReflector::resizeArray(std::size_t newSize)
{
    ensureArray(slot).getVector().resize(newSize);
}

Vector<std::string> JsonReflector::mapKeys() const
{
    auto keys = Vector<std::string> {};

    if (!slot.isObject())
        return keys;

    for (auto& [key, _]: slot.asObject())
        keys.add(key);

    return keys;
}

} // namespace Miro
