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
            return "integer";
    }
    return "string";
}

// Renders a single TypeNode as a JSON Schema fragment. Named types
// (Object, Enum) become {"$ref": "#/$defs/<typeName>"}; the bodies live
// in $defs and are emitted separately. Anonymous nodes (top-level
// primitives, vectors, maps with no named root) get an inline body.
Json::Value renderJsonNode(const TypeNode& node);

void writeRequired(const TypeNode& node, Json::Value& out)
{
    auto required = Json::Array {};

    for (auto& field: node.fields)
        if (!field.type->optional)
            required.emplace_back(field.name);

    if (!required.empty())
        out.asObject()["required"] = Json::Value {std::move(required)};
}

Json::Value renderJsonObjectBody(const TypeNode& node)
{
    auto out = Json::Value {Json::Object {}};
    out.asObject()["type"] = Json::Value {std::string {"object"}};

    auto props = Json::Value {Json::Object {}};
    for (auto& field: node.fields)
        props.asObject()[field.name] = renderJsonNode(*field.type);

    out.asObject()["properties"] = std::move(props);

    writeRequired(node, out);

    return out;
}

Json::Value renderJsonEnumBody(const TypeNode& node)
{
    auto out = Json::Value {Json::Object {}};
    auto& obj = out.asObject();
    obj["type"] = Json::Value {std::string {"string"}};

    if (!node.enumValues.empty())
    {
        auto values = Json::Array {};
        values.reserve(node.enumValues.size());
        for (auto& v: node.enumValues)
            values.emplace_back(v);
        obj["enum"] = Json::Value {std::move(values)};
    }

    return out;
}

Json::Value renderJsonNode(const TypeNode& node)
{
    auto out = Json::Value {Json::Object {}};

    auto& obj = out.asObject();

    auto applyOptional = [&]
    {
        if (node.optional)
            obj["nullable"] = Json::Value {true};
    };

    switch (node.shape)
    {
        case TypeNode::Shape::Primitive:
        {
            obj["type"] =
                Json::Value {std::string {jsonSchemaPrimitive(node.primitive)}};

            applyOptional();
            return out;
        }

        case TypeNode::Shape::Object:
        {
            if (!node.typeName.empty())
            {
                obj["$ref"] = Json::Value {std::string {"#/$defs/"} + node.typeName};
                applyOptional();
                return out;
            }
            // Anonymous object — render inline.
            out = renderJsonObjectBody(node);
            applyOptional();
            return out;
        }

        case TypeNode::Shape::Array:
        {
            obj["type"] = Json::Value {std::string {"array"}};
            obj["items"] = renderJsonNode(*node.inner);

            if (node.minItems)
                obj["minItems"] = Json::Value {static_cast<double>(*node.minItems)};
            if (node.maxItems)
                obj["maxItems"] = Json::Value {static_cast<double>(*node.maxItems)};
            applyOptional();
            return out;
        }

        case TypeNode::Shape::Map:
        {
            obj["type"] = Json::Value {std::string {"object"}};
            obj["additionalProperties"] = renderJsonNode(*node.inner);
            applyOptional();
            return out;
        }

        case TypeNode::Shape::Enum:
        {
            if (!node.typeName.empty())
            {
                obj["$ref"] = Json::Value {std::string {"#/$defs/"} + node.typeName};
                applyOptional();
                return out;
            }
            // Anonymous enum (shouldn't happen — visitEnum always sets a
            // name — but render inline if it does).
            out = renderJsonEnumBody(node);
            applyOptional();
            return out;
        }
    }

    return out;
}

} // namespace

Json::Value formatJsonSchema(std::span<TypeNode> roots)
{
    auto ordered = TypeTree::prepareRoots(roots);

    // Build $defs first so root references resolve correctly.
    auto defs = Json::Value {Json::Object {}};
    for (auto* node: ordered)
    {
        auto& obj = defs.asObject()[node->typeName];

        if (node->shape == TypeNode::Shape::Enum)
            obj = renderJsonEnumBody(*node);
        else
            obj = renderJsonObjectBody(*node);
    }

    // The single-root contract: render the root inline, then attach
    // $defs alongside if non-empty. The multi-root span overload is used
    // by the type-export runner; we still produce a single object, but
    // there's no obvious "the root" — treat the first root as the
    // primary schema and ignore the rest beyond their $defs entries.
    auto root = roots.empty() ? Json::Value {Json::Object {}}
                              : renderJsonNode(roots.front());

    if (!defs.asObject().empty())
    {
        if (!root.isObject())
            root = Json::Value {Json::Object {}};
        root.asObject()["$defs"] = std::move(defs);
    }

    return root;
}

Json::Value formatJsonSchema(TypeNode& root)
{
    return formatJsonSchema(std::span {&root, 1});
}

} // namespace Miro
