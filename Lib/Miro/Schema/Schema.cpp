#include "Schema.h"

#include <string>
#include <string_view>
#include <utility>

namespace Miro
{

using TypeTree::TypeNode;

namespace
{

std::string_view jsonSchemaPrimitive(TypeTree::PrimitiveKind kind)
{
    switch (kind)
    {
        case TypeTree::PrimitiveKind::Boolean:
            return "boolean";
        case TypeTree::PrimitiveKind::String:
            return "string";
        case TypeTree::PrimitiveKind::Number:
            return "number";
        case TypeTree::PrimitiveKind::Integer:
        case TypeTree::PrimitiveKind::Int64:
            return "integer";
    }
    return "string";
}

JSON renderJsonNode(const TypeNode& node);

void writeRequired(const TypeNode& node, JSON& out)
{
    auto required = Json::Array {};

    for (auto& field: node.fields)
        if (!field.type->optional)
            required.emplace_back(field.name);

    if (!required.empty())
        out.asObject()["required"] = JSON {std::move(required)};
}

JSON renderJsonObjectBody(const TypeNode& node)
{
    auto out = JSON {Json::Object {}};
    out.asObject()["type"] = JSON {std::string {"object"}};

    auto props = JSON {Json::Object {}};
    for (auto& field: node.fields)
        props.asObject()[field.name] = renderJsonNode(*field.type);

    out.asObject()["properties"] = std::move(props);

    writeRequired(node, out);

    return out;
}

JSON renderJsonEnumBody(const TypeNode& node)
{
    auto out = JSON {Json::Object {}};
    auto& obj = out.asObject();
    obj["type"] = JSON {std::string {"string"}};

    if (!node.enumValues.empty())
    {
        auto values = Json::Array {};
        values.reserve(node.enumValues.size());
        for (auto& v: node.enumValues)
            values.emplace_back(v);
        obj["enum"] = JSON {std::move(values)};
    }

    return out;
}

void markNullable(JSON& out, bool optional)
{
    if (optional)
        out.asObject()["nullable"] = JSON {true};
}

JSON makeRefTo(const std::string& typeName)
{
    auto out = JSON {Json::Object {}};
    out.asObject()["$ref"] = JSON {std::string {"#/$defs/"} + typeName};
    return out;
}

JSON renderJsonPrimitive(const TypeNode& node)
{
    auto out = JSON {Json::Object {}};
    out.asObject()["type"] =
        JSON {std::string {jsonSchemaPrimitive(node.primitive)}};
    return out;
}

JSON renderJsonArrayBody(const TypeNode& node)
{
    auto out = JSON {Json::Object {}};
    auto& obj = out.asObject();

    obj["type"] = JSON {std::string {"array"}};
    obj["items"] = renderJsonNode(*node.inner);

    if (node.minItems)
        obj["minItems"] = JSON {static_cast<double>(*node.minItems)};
    if (node.maxItems)
        obj["maxItems"] = JSON {static_cast<double>(*node.maxItems)};

    return out;
}

JSON renderJsonMapBody(const TypeNode& node)
{
    auto out = JSON {Json::Object {}};
    auto& obj = out.asObject();

    obj["type"] = JSON {std::string {"object"}};
    obj["additionalProperties"] = renderJsonNode(*node.inner);

    return out;
}

JSON renderJsonNode(const TypeNode& node)
{
    auto out = [&]
    {
        switch (node.shape)
        {
            case TypeNode::Shape::Primitive:
                return renderJsonPrimitive(node);

            case TypeNode::Shape::Object:
                return node.typeName.empty() ? renderJsonObjectBody(node)
                                             : makeRefTo(node.typeName);

            case TypeNode::Shape::Array:
                return renderJsonArrayBody(node);

            case TypeNode::Shape::Map:
                return renderJsonMapBody(node);

            case TypeNode::Shape::Enum:
                // visitEnum always names enums; inline is just a fallback.
                return node.typeName.empty() ? renderJsonEnumBody(node)
                                             : makeRefTo(node.typeName);
        }

        return JSON {Json::Object {}};
    }();

    markNullable(out, node.optional);
    return out;
}

} // namespace

namespace
{

JSON buildDefs(std::span<TypeNode> roots)
{
    auto ordered = TypeTree::prepareRoots(roots);

    auto defs = JSON {Json::Object {}};
    for (auto* node: ordered)
    {
        auto& obj = defs.asObject()[node->typeName];

        if (node->shape == TypeNode::Shape::Enum)
            obj = renderJsonEnumBody(*node);
        else
            obj = renderJsonObjectBody(*node);
    }

    return defs;
}

} // namespace

JSON formatJsonSchema(std::span<TypeNode> roots)
{
    auto defs = buildDefs(roots);

    auto out = JSON {Json::Object {}};
    if (!defs.asObject().empty())
        out.asObject()["$defs"] = std::move(defs);

    return out;
}

JSON formatJsonSchema(TypeNode& root)
{
    // $defs first: prepareRoots rewrites typeName, and rendering the root
    // needs the rewritten names.
    auto defs = buildDefs(std::span {&root, 1});

    auto out = renderJsonNode(root);

    if (!defs.asObject().empty())
    {
        if (!out.isObject())
            out = JSON {Json::Object {}};
        out.asObject()["$defs"] = std::move(defs);
    }

    return out;
}

} // namespace Miro
