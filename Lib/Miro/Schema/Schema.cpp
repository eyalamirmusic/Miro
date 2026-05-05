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

JSON renderJsonNode(const TypeNode& node)
{
    auto out = JSON {Json::Object {}};

    auto& obj = out.asObject();

    auto applyOptional = [&]
    {
        if (node.optional)
            obj["nullable"] = JSON {true};
    };

    switch (node.shape)
    {
        case TypeNode::Shape::Primitive:
        {
            obj["type"] =
                JSON {std::string {jsonSchemaPrimitive(node.primitive)}};

            applyOptional();
            return out;
        }

        case TypeNode::Shape::Object:
        {
            if (!node.typeName.empty())
            {
                obj["$ref"] = JSON {std::string {"#/$defs/"} + node.typeName};
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
            obj["type"] = JSON {std::string {"array"}};
            obj["items"] = renderJsonNode(*node.inner);

            if (node.minItems)
                obj["minItems"] = JSON {static_cast<double>(*node.minItems)};
            if (node.maxItems)
                obj["maxItems"] = JSON {static_cast<double>(*node.maxItems)};
            applyOptional();
            return out;
        }

        case TypeNode::Shape::Map:
        {
            obj["type"] = JSON {std::string {"object"}};
            obj["additionalProperties"] = renderJsonNode(*node.inner);
            applyOptional();
            return out;
        }

        case TypeNode::Shape::Enum:
        {
            if (!node.typeName.empty())
            {
                obj["$ref"] = JSON {std::string {"#/$defs/"} + node.typeName};
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

namespace
{

// Walks every named type reachable from the roots and returns a $defs
// object. Mutates roots in place via prepareRoots — the per-node
// disambiguation pass rewrites typeName when distinct types share a
// short name.
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

// Multi-root variant — used by the type-export runner to bundle every
// registered type into a single document. No type is privileged as
// "the schema"; consumers point at one with {"$ref": "#/$defs/X"}.
JSON formatJsonSchema(std::span<TypeNode> roots)
{
    auto defs = buildDefs(roots);

    auto out = JSON {Json::Object {}};
    if (!defs.asObject().empty())
        out.asObject()["$defs"] = std::move(defs);

    return out;
}

// Single-root: render the root (a $ref for named types, an inline body
// for top-level vectors / primitives) and attach $defs alongside. Used
// by `schemaOf<T>()`.
JSON formatJsonSchema(TypeNode& root)
{
    // Build $defs first — prepareRoots rewrites typeName during this
    // pass, and we need the renamed names when we render the root.
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
