#include "Swift.h"

#include "../Detail/StringUtilities.h"
#include "SwiftNaming.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Miro::Swift
{

using TypeTree::TypeNode;

namespace
{

std::string_view swiftPrimitive(TypeTree::PrimitiveKind kind)
{
    switch (kind)
    {
        case TypeTree::PrimitiveKind::Boolean:
            return "Bool";
        case TypeTree::PrimitiveKind::String:
            return "String";
        case TypeTree::PrimitiveKind::Number:
            return "Double";
        case TypeTree::PrimitiveKind::Integer:
            return "Int";
        case TypeTree::PrimitiveKind::Int64:
            return "Int64";
    }
    return "String";
}

std::string renderType(const TypeNode& node);

// Wraps `renderType(node)` in a Swift optional (`T?`) when the node is
// nullable. Used for fields and inner element types.
std::string renderTypeWithOptional(const TypeNode& node)
{
    auto base = renderType(node);
    if (node.optional)
        return base + "?";
    return base;
}

std::string renderType(const TypeNode& node)
{
    switch (node.shape)
    {
        case TypeNode::Shape::Primitive:
            return std::string {swiftPrimitive(node.primitive)};
        case TypeNode::Shape::Object:
        case TypeNode::Shape::Enum:
            return node.typeName;
        case TypeNode::Shape::Array:
            return "[" + renderTypeWithOptional(*node.inner) + "]";
        case TypeNode::Shape::Map:
            return "[String: " + renderTypeWithOptional(*node.inner) + "]";
    }
    return "String";
}

// How one field is spelled in Swift: the property declaration name (may
// be backtick-escaped), the original JSON wire key, and whether the two
// differ — when they do the enclosing struct needs a CodingKeys map.
struct FieldNaming
{
    std::string property;
    std::string jsonKey;
    bool remapped = false;
};

// Sanitizes an arbitrary JSON key into a valid Swift property name:
// characters that aren't identifier-legal become '_', and a leading
// non-start character (e.g. a digit) gets an '_' prefix. Always paired
// with a CodingKeys entry that restores the original wire key.
std::string sanitizeFieldName(std::string_view name)
{
    auto out = std::string {};

    for (auto i = std::size_t {0}; i < name.size(); ++i)
    {
        auto c = name[i];
        auto ok =
            (i == 0) ? Detail::isAsciiIdentStart(c) : Detail::isAsciiIdentPart(c);
        out += ok ? c : '_';
    }

    if (out.empty())
        out = "_";

    return out;
}

FieldNaming nameField(std::string_view jsonKey)
{
    auto naming = FieldNaming {};
    naming.jsonKey = std::string {jsonKey};

    // A bare identifier (keyword or not) maps to itself: Codable's
    // synthesized coding key is the property name without backticks, so
    // it already equals the wire key and needs no CodingKeys entry.
    if (Detail::isAsciiIdentifier(jsonKey))
    {
        naming.property = Naming::swiftIdentifier(jsonKey);
        naming.remapped = false;
    }
    else
    {
        naming.property = sanitizeFieldName(jsonKey);
        naming.remapped = true;
    }

    return naming;
}

void emitCodingKeys(std::ostringstream& out,
                    const TypeNode& node,
                    const std::vector<FieldNaming>& namings)
{
    out << "\n    enum CodingKeys: String, CodingKey {\n";

    auto index = std::size_t {0};
    for ([[maybe_unused]] auto& field: node.fields)
    {
        auto& naming = namings[index++];
        out << "        case " << naming.property << " = \""
            << Naming::escapeSwiftString(naming.jsonKey) << "\"\n";
    }

    out << "    }\n";
}

std::string emitStruct(const TypeNode& node)
{
    auto namings = std::vector<FieldNaming> {};
    auto needsCodingKeys = false;

    for (auto& field: node.fields)
    {
        auto naming = nameField(field.name);
        needsCodingKeys = needsCodingKeys || naming.remapped;
        namings.push_back(std::move(naming));
    }

    auto out = std::ostringstream {};
    out << "struct " << node.typeName << ": Codable {\n";

    auto index = std::size_t {0};
    for (auto& field: node.fields)
    {
        auto& naming = namings[index++];
        auto suffix = field.type->optional ? " = nil" : "";
        out << "    var " << naming.property << ": "
            << renderTypeWithOptional(*field.type) << suffix << "\n";
    }

    if (needsCodingKeys)
        emitCodingKeys(out, node, namings);

    out << "}\n";
    return out.str();
}

std::string emitEnum(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "enum " << node.typeName << ": String, Codable {\n";

    for (auto& value: node.enumValues)
        out << "    case " << Naming::swiftIdentifier(value) << " = \""
            << Naming::escapeSwiftString(value) << "\"\n";

    out << "}\n";
    return out.str();
}

} // namespace

std::string formatTypes(std::span<TypeNode> roots)
{
    auto ordered = TypeTree::prepareRoots(roots);

    auto out = std::ostringstream {};

    for (auto* node: ordered)
    {
        if (node->shape == TypeNode::Shape::Enum)
            out << emitEnum(*node) << "\n";
        else
            out << emitStruct(*node) << "\n";
    }

    return out.str();
}

std::string formatTypes(TypeNode& root)
{
    return formatTypes(std::span<TypeNode> {&root, 1});
}

} // namespace Miro::Swift
