#pragma once

#include "ReflectContainers.h"
#include "ReflectDispatch.h"
#include "TypeName.h"
#include "XmlReflector.h"

#include <string>
#include <string_view>

namespace Miro
{

template <typename T>
Xml::Node toXML(const T& value)
{
    auto root = Xml::Node {.name = std::string(Detail::typeNameOf<T>())};
    auto ref = XmlReflector {root, Detail::topLevelOptions<T>(Mode::Save)};
    Detail::reflectValue(ref, const_cast<T&>(value));
    return root;
}

template <typename T>
void fromXML(T& value, const Xml::Node& node)
{
    auto mutableNode = node;
    auto ref = XmlReflector {mutableNode, Detail::topLevelOptions<T>(Mode::Load)};
    Detail::reflectValue(ref, value);
}

template <typename T>
T createFromXML(const Xml::Node& node)
{
    auto value = T {};
    fromXML(value, node);
    return value;
}

template <typename T>
std::string toXMLString(const T& value, int indent = 0)
{
    return Xml::print(toXML(value), indent);
}

template <typename T>
void fromXMLString(T& value, std::string_view xmlString)
{
    fromXML(value, Xml::parse(xmlString));
}

template <typename T>
T createFromXMLString(std::string_view xmlString)
{
    return createFromXML<T>(Xml::parse(xmlString));
}

} // namespace Miro
