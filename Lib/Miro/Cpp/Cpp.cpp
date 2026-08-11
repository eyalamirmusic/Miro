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
        case TypeTree::PrimitiveKind::Int64:
            return "std::int64_t";
    }
    return "int";
}

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
        case TypeTree::PrimitiveKind::Int64:
            return " = 0";
        case TypeTree::PrimitiveKind::String:
            return {};
    }

    return {};
}

std::string renderType(const TypeNode& node);

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

void emitStructFields(std::ostringstream& out, const TypeNode& node)
{
    for (auto& field: node.fields)
        out << "    " << renderTypeWithOptional(*field.type) << " " << field.name
            << defaultInitFor(*field.type) << ";\n";
}

void emitReflectMacro(std::ostringstream& out, const TypeNode& node)
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

std::string emitStruct(const TypeNode& node, Modes mode)
{
    auto out = std::ostringstream {};
    out << "struct " << node.typeName << "\n{\n";

    emitStructFields(out, node);

    if (mode == Modes::Miro)
        emitReflectMacro(out, node);

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

std::string formatHeader(std::span<TypeNode> roots, Modes mode)
{
    auto ordered = TypeTree::prepareRoots(roots);

    auto out = std::ostringstream {};
    out << "#pragma once\n\n";
    out << "#include <cstdint>\n";
    out << "#include <map>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <vector>\n";

    if (mode == Modes::Miro)
        out << "\n#include <Miro/Miro.h>\n";

    out << "\n";

    for (auto* node: ordered)
    {
        if (node->shape == TypeNode::Shape::Enum)
            out << emitEnum(*node) << "\n";
        else
            out << emitStruct(*node, mode) << "\n";
    }

    return out.str();
}

std::string formatHeader(TypeNode& root, Modes mode)
{
    return formatHeader(std::span<TypeNode> {&root, 1}, mode);
}

} // namespace Miro::Cpp
