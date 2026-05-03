#include "Cpp.h"

#include <sstream>
#include <string>
#include <string_view>

namespace Miro::Cpp
{

using TypeTree::TypeNode;

namespace
{

std::string_view cppPrimitive(TypeTree::PrimitiveKind kind)
{
    switch (kind)
    {
        case TypeTree::PrimitiveKind::Boolean:
            return "bool";
        case TypeTree::PrimitiveKind::String:
            return "std::string";
        case TypeTree::PrimitiveKind::Number:
            return "double";
        case TypeTree::PrimitiveKind::Integer:
            return "int";
    }
    return "int";
}

// Default initializer for primitive fields so the generated structs
// produce predictable values without explicit construction. Containers
// and optionals are already empty by default; named types fall back to
// the user's own default constructor.
std::string defaultInitFor(const TypeNode& field)
{
    if (field.optional)
        return {};

    if (field.shape != TypeNode::Shape::Primitive)
        return {};

    switch (field.primitive)
    {
        case TypeTree::PrimitiveKind::Boolean:
            return " = false";
        case TypeTree::PrimitiveKind::Number:
        case TypeTree::PrimitiveKind::Integer:
            return " = 0";
        case TypeTree::PrimitiveKind::String:
            return {};
    }

    return {};
}

std::string renderType(const TypeNode& node);

// Wraps `renderType(node)` in `std::optional<...>` when the node is
// nullable. Used for fields and inner element types.
std::string renderTypeWithOptional(const TypeNode& node)
{
    auto base = renderType(node);
    if (node.optional)
        return "std::optional<" + base + ">";
    return base;
}

std::string renderType(const TypeNode& node)
{
    switch (node.shape)
    {
        case TypeNode::Shape::Primitive:
            return std::string {cppPrimitive(node.primitive)};
        case TypeNode::Shape::Object:
        case TypeNode::Shape::Enum:
            return node.typeName;
        case TypeNode::Shape::Array:
            return "std::vector<" + renderTypeWithOptional(*node.inner) + ">";
        case TypeNode::Shape::Map:
            return "std::map<std::string, " + renderTypeWithOptional(*node.inner)
                   + ">";
    }
    return "auto";
}

std::string emitStruct(const TypeNode& node, bool withMiro)
{
    auto out = std::ostringstream {};
    out << "struct " << node.typeName << "\n{\n";

    for (auto& field: node.fields)
        out << "    " << renderTypeWithOptional(*field.type) << " " << field.name
            << defaultInitFor(*field.type) << ";\n";

    if (withMiro)
    {
        if (!node.fields.empty())
            out << "\n";

        out << "    MIRO_REFLECT(";
        auto first = true;
        for (auto& field: node.fields)
        {
            if (!first)
                out << ", ";
            first = false;
            out << field.name;
        }
        out << ")\n";
    }

    out << "};\n";
    return out.str();
}

std::string emitEnum(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "enum class " << node.typeName << "\n{\n";

    auto first = true;
    for (auto& v: node.enumValues)
    {
        if (!first)
            out << ",\n";
        first = false;
        out << "    " << v;
    }
    out << "\n};\n";

    return out.str();
}

} // namespace

std::string formatHeader(std::span<TypeNode> roots, bool withMiro)
{
    auto ordered = TypeTree::prepareRoots(roots);

    auto out = std::ostringstream {};
    out << "#pragma once\n\n";
    out << "#include <map>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <vector>\n";

    if (withMiro)
        out << "\n#include <Miro/Miro.h>\n";

    out << "\n";

    for (auto* node: ordered)
    {
        if (node->shape == TypeNode::Shape::Enum)
            out << emitEnum(*node) << "\n";
        else
            out << emitStruct(*node, withMiro) << "\n";
    }

    return out.str();
}

std::string formatHeader(TypeNode& root, bool withMiro)
{
    return formatHeader(std::span<TypeNode> {&root, 1}, withMiro);
}

} // namespace Miro::Cpp
