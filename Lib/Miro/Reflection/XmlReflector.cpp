#include "XmlReflector.h"

#include <cmath>
#include <concepts>
#include <cstdlib>
#include <sstream>
#include <type_traits>

namespace Miro
{
namespace
{

template <typename T>
std::string primitiveToString(T* ptr)
{
    if constexpr (std::same_as<T, bool>)
    {
        return *ptr ? "true" : "false";
    }
    else if constexpr (std::same_as<T, std::string>)
    {
        return *ptr;
    }
    else if constexpr (std::same_as<T, double>)
    {
        // Matches the JSON printer: whole numbers print without a decimal
        // point, so int-backed reflect targets round-trip cleanly.
        if (std::isfinite(*ptr) && *ptr == std::floor(*ptr)
            && std::abs(*ptr) < 1e15)
        {
            auto stream = std::ostringstream {};
            stream << static_cast<long long>(*ptr);
            return stream.str();
        }

        auto stream = std::ostringstream {};
        stream << *ptr;
        return stream.str();
    }
    else
    {
        return std::to_string(*ptr);
    }
}

template <typename T>
void primitiveFromString(const std::string& text, T* ptr)
{
    if constexpr (std::same_as<T, bool>)
    {
        *ptr = (text == "true" || text == "1");
    }
    else if constexpr (std::same_as<T, std::string>)
    {
        *ptr = text;
    }
    else if constexpr (std::same_as<T, double>)
    {
        *ptr = std::strtod(text.c_str(), nullptr);
    }
    else
    {
        *ptr = static_cast<T>(std::strtoll(text.c_str(), nullptr, 10));
    }
}

std::size_t countSiblings(const Xml::Node& parent, std::string_view name)
{
    auto count = std::size_t {0};

    for (const auto& child: parent.children)
        if (child.name == name)
            ++count;

    return count;
}

Xml::Node* findNthSibling(Xml::Node& parent, std::string_view name, std::size_t i)
{
    auto count = std::size_t {0};

    for (auto& child: parent.children)
    {
        if (child.name != name)
            continue;

        if (count == i)
            return &child;

        ++count;
    }

    return nullptr;
}

} // namespace

XmlReflector::XmlReflector(Xml::Node& rootToUse, Options optsToUse)
    : XmlReflector(ElementSlot {&rootToUse}, optsToUse)
{
}

XmlReflector::XmlReflector(Slot slotToUse, Options optsToUse)
    : Reflector(optsToUse)
    , slot(std::move(slotToUse))
{
}

XmlReflector::~XmlReflector() = default;

ValueKind XmlReflector::kind() const
{
    if (std::holds_alternative<MissingSlot>(slot))
        return ValueKind::Absent;

    if (std::holds_alternative<AttributeSlot>(slot))
        return ValueKind::String;

    if (std::holds_alternative<ArraySlot>(slot))
        return ValueKind::Array;

    return ValueKind::Object;
}

void XmlReflector::writeNull()
{
    // Nothing to write: attributes and array siblings only appear once
    // something visits them, and an empty element is XML's closest null.
}

void XmlReflector::visit(PrimitiveRef ref)
{
    std::visit(
        [this, ref](auto& s)
        {
            using S = std::decay_t<decltype(s)>;

            if constexpr (std::same_as<S, ElementSlot>)
            {
                std::visit(
                    [this, &s](auto* ptr)
                    {
                        if (isSaving())
                            s.node->text = primitiveToString(ptr);
                        else
                            primitiveFromString(s.node->text, ptr);
                    },
                    ref.data);
            }
            else if constexpr (std::same_as<S, AttributeSlot>)
            {
                std::visit(
                    [this, &s](auto* ptr)
                    {
                        if (isSaving())
                            s.parent->attributes[s.name] = primitiveToString(ptr);
                        else if (auto it = s.parent->attributes.find(s.name);
                                 it != s.parent->attributes.end())
                            primitiveFromString(it->second, ptr);
                    },
                    ref.data);
            }
        },
        slot);
}

Reflector& XmlReflector::atKey(std::string_view key, Options childOpts)
{
    return std::visit(
        [&](auto& s) -> Reflector&
        {
            using S = std::decay_t<decltype(s)>;

            if constexpr (std::same_as<S, ElementSlot>)
                return atKeyOnElement(*s.node, key, childOpts);
            else
                return spawnMissing(childOpts);
        },
        slot);
}

Reflector& XmlReflector::atKeyOnElement(Xml::Node& parent,
                                       std::string_view key,
                                       Options childOpts)
{
    auto keyStr = std::string(key);

    switch (childOpts.shape)
    {
        case Shape::Primitive:
            if (isLoading() && !parent.attributes.contains(keyStr))
                return spawnMissing(childOpts);

            return spawnAttribute(parent, std::move(keyStr), childOpts);

        case Shape::Array:
            return spawnArray(parent, std::move(keyStr), childOpts);

        case Shape::Object:
        case Shape::Map:
            if (isSaving())
            {
                parent.children.add(Xml::Node {.name = keyStr});
                return spawnElement(parent.children.back(), childOpts);
            }

            if (auto* child = Xml::findChild(parent, keyStr))
                return spawnElement(*child, childOpts);

            return spawnMissing(childOpts);
    }

    return spawnMissing(childOpts);
}

Reflector& XmlReflector::atIndex(std::size_t index, Options childOpts)
{
    return std::visit(
        [&](auto& s) -> Reflector&
        {
            using S = std::decay_t<decltype(s)>;

            if constexpr (std::same_as<S, ArraySlot>)
                return atIndexOnArray(*s.parent, s.elementName, index, childOpts);
            else
                return spawnMissing(childOpts);
        },
        slot);
}

Reflector& XmlReflector::atIndexOnArray(Xml::Node& parent,
                                       const std::string& elementName,
                                       std::size_t index,
                                       Options childOpts)
{
    if (isSaving())
    {
        auto count = countSiblings(parent, elementName);

        while (count <= index)
        {
            parent.children.add(Xml::Node {.name = elementName});
            ++count;
        }
    }

    auto* node = findNthSibling(parent, elementName, index);

    if (node == nullptr)
        return spawnMissing(childOpts);

    // A nested array has no field name for its inner axis.
    if (childOpts.shape == Shape::Array)
        return spawnArray(*node, "item", childOpts);

    return spawnElement(*node, childOpts);
}

std::size_t XmlReflector::arraySize() const
{
    if (const auto* s = std::get_if<ArraySlot>(&slot))
        return countSiblings(*s->parent, s->elementName);

    return 0;
}

void XmlReflector::resizeArray(std::size_t newSize)
{
    auto* s = std::get_if<ArraySlot>(&slot);

    if (s == nullptr)
        return;

    auto& children = s->parent->children;
    auto count = countSiblings(*s->parent, s->elementName);

    while (count > newSize)
    {
        for (auto it = children.begin(); it != children.end(); ++it)
        {
            if (it->name == s->elementName)
            {
                children.erase(it);
                --count;
                break;
            }
        }
    }

    while (count < newSize)
    {
        children.add(Xml::Node {.name = s->elementName});
        ++count;
    }
}

Vector<std::string> XmlReflector::mapKeys() const
{
    auto keys = Vector<std::string> {};

    const auto* s = std::get_if<ElementSlot>(&slot);

    if (s == nullptr)
        return keys;

    for (const auto& [key, _]: s->node->attributes)
        keys.add(key);

    for (const auto& child: s->node->children)
    {
        auto seen = false;

        for (const auto& existing: keys)
        {
            if (existing == child.name)
            {
                seen = true;
                break;
            }
        }

        if (!seen)
            keys.add(child.name);
    }

    return keys;
}

Reflector& XmlReflector::spawnElement(Xml::Node& node, Options childOpts)
{
    currentChild.reset();
    currentChild = new XmlReflector(ElementSlot {&node}, childOpts);
    return *currentChild;
}

Reflector& XmlReflector::spawnAttribute(Xml::Node& parent,
                                       std::string name,
                                       Options childOpts)
{
    currentChild.reset();
    currentChild =
        new XmlReflector(AttributeSlot {&parent, std::move(name)}, childOpts);
    return *currentChild;
}

Reflector& XmlReflector::spawnArray(Xml::Node& parent,
                                    std::string name,
                                    Options childOpts)
{
    currentChild.reset();
    currentChild =
        new XmlReflector(ArraySlot {&parent, std::move(name)}, childOpts);
    return *currentChild;
}

Reflector& XmlReflector::spawnMissing(Options childOpts)
{
    currentChild.reset();
    currentChild = new XmlReflector(MissingSlot {}, childOpts);
    return *currentChild;
}

} // namespace Miro
