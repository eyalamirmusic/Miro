#include "Schema.h"

#include "../Detail/StringUtilities.h"

#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace Miro::Schema
{

Dialect Dialect::standard()
{
    return Dialect {};
}

Dialect Dialect::openAiStrict()
{
    auto dialect = Dialect {};
    dialect.closedObjects = true;
    dialect.allRequired = true;
    dialect.stripExtensions = true;
    dialect.nullStyle = NullStyle::TypeUnion;
    dialect.unionStyle = UnionStyle::AnyOf;
    return dialect;
}

} // namespace Miro::Schema

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

JSON makeObject()
{
    return JSON {Json::Object {}};
}

JSON makeString(std::string_view text)
{
    return JSON {std::string {text}};
}

JSON makeRefTo(const std::string& typeName)
{
    auto out = makeObject();
    out.asObject()["$ref"] = JSON {std::string {"#/$defs/"} + typeName};
    return out;
}

JSON makeNullType()
{
    auto out = makeObject();
    out.asObject()["type"] = makeString("null");
    return out;
}

std::string fieldPath(const std::string& parent, std::string_view field)
{
    auto out = parent;
    out += '.';
    out += field;
    return out;
}

// Renders TypeNodes as JSON Schema fragments in one Dialect. Named
// types (Object, Enum, Union) become {"$ref": "#/$defs/<typeName>"} at
// their use sites; their bodies are rendered once by declaration() into
// $defs. `path` threads the dotted field path down for error messages.
class Renderer
{
public:
    Renderer(const Schema::Dialect& dialectToUse,
             const Vector<const TypeNode*>& declaredToUse)
        : dialect(dialectToUse)
    {
        for (auto* node: declaredToUse)
            if (!node->qualifiedName.empty())
                declarations[node->qualifiedName] = node;
    }

    // A slot: the value's schema plus whatever belongs to the slot —
    // nullability and the field description. A named type's body never
    // goes through here, so neither leaks into a shared definition.
    JSON slot(const TypeNode& node, const std::string& path) const
    {
        auto out = value(node, path);

        if (isNullable(node))
            out = withNull(std::move(out));

        if (!node.description.empty())
            out.asObject()["description"] = JSON {node.description};

        return out;
    }

    // The $defs body of a named type.
    JSON declaration(const TypeNode& node) const
    {
        if (node.shape == TypeNode::Shape::Enum)
            return enumBody(node);

        if (node.shape == TypeNode::Shape::Union)
            return unionBody(node, node.typeName);

        return objectBody(node, node.typeName);
    }

private:
    bool isNullable(const TypeNode& node) const
    {
        return node.optional || (dialect.allRequired && node.omittable);
    }

    bool isRequired(const TypeNode& node) const
    {
        return dialect.allRequired || (!node.optional && !node.omittable);
    }

    JSON withNull(JSON out) const
    {
        if (dialect.nullStyle == Schema::Dialect::NullStyle::Keyword)
        {
            out.asObject()["nullable"] = JSON {true};
            return out;
        }

        // A plain `"type": "x"` widens in place. Anything else — a $ref,
        // an enum list (which would reject null on its own), a
        // composition — gets wrapped.
        auto& obj = out.asObject();
        auto* type = Json::find(obj, "type");

        if (type != nullptr && type->isString() && !obj.contains("enum"))
        {
            auto types = Json::Array {};
            types.emplace_back(type->asString());
            types.emplace_back(std::string {"null"});
            *type = JSON {std::move(types)};
            return out;
        }

        auto options = Json::Array {};
        options.emplace_back(std::move(out));
        options.emplace_back(makeNullType());

        auto wrapped = makeObject();
        wrapped.asObject()["anyOf"] = JSON {std::move(options)};
        return wrapped;
    }

    [[noreturn]] void throwUnsupported(std::string_view what,
                                       const std::string& path) const
    {
        throw std::invalid_argument(
            "JSON Schema dialect with closed objects cannot express "
            + std::string {what} + " (at '" + path + "')");
    }

    JSON value(const TypeNode& node, const std::string& path) const
    {
        switch (node.shape)
        {
            case TypeNode::Shape::Primitive:
                return primitive(node);

            case TypeNode::Shape::Object:
                return node.typeName.empty() ? objectBody(node, path)
                                             : makeRefTo(node.typeName);

            case TypeNode::Shape::Array:
                return arrayBody(node, path);

            case TypeNode::Shape::Map:
                return mapBody(node, path);

            case TypeNode::Shape::Enum:
                // Anonymous enum shouldn't happen — visitEnum always sets
                // a name — but render inline if it does.
                return node.typeName.empty() ? enumBody(node)
                                             : makeRefTo(node.typeName);

            case TypeNode::Shape::Any:
                if (dialect.closedObjects)
                    throwUnsupported("a raw JSON value", path);

                // The empty schema is JSON Schema's "anything goes".
                return makeObject();

            case TypeNode::Shape::Union:
                return node.typeName.empty() ? unionBody(node, path)
                                             : makeRefTo(node.typeName);
        }

        return makeObject();
    }

    JSON primitive(const TypeNode& node) const
    {
        auto out = makeObject();
        out.asObject()["type"] = makeString(jsonSchemaPrimitive(node.primitive));
        return out;
    }

    void addProperties(const Vector<TypeNode::Field>& fields,
                       const std::string& path,
                       Json::Object& props,
                       Json::Array& required) const
    {
        for (auto& field: fields)
        {
            props[field.name] = slot(*field.type, fieldPath(path, field.name));

            if (isRequired(*field.type))
                addRequired(required, field.name);
        }
    }

    static void addRequired(Json::Array& required, const std::string& name)
    {
        for (auto& existing: required)
            if (existing.asString() == name)
                return;

        required.emplace_back(name);
    }

    // type / properties / required (/ additionalProperties) — the common
    // tail of an object body and a flattened union arm.
    JSON objectFrom(Json::Object props, Json::Array required) const
    {
        auto out = makeObject();
        auto& obj = out.asObject();
        obj["type"] = makeString("object");
        obj["properties"] = JSON {std::move(props)};

        // An empty list has always been left out; a dialect that lists
        // every property states the empty one explicitly.
        if (!required.empty() || dialect.allRequired)
            obj["required"] = JSON {std::move(required)};

        if (dialect.closedObjects)
            obj["additionalProperties"] = JSON {false};

        return out;
    }

    JSON objectBody(const TypeNode& node, const std::string& path) const
    {
        auto props = Json::Object {};
        auto required = Json::Array {};
        addProperties(node.fields, path, props, required);
        return objectFrom(std::move(props), std::move(required));
    }

    // Names of an integer enum's members. JSON Schema has no keyword for
    // them, so they go out under the conventional "x-enumNames"
    // extension — validators ignore it, code generators can pick the
    // names back up.
    void writeEnumNames(const TypeNode& node, Json::Object& obj) const
    {
        if (dialect.stripExtensions)
            return;

        auto names = Json::Array {};
        names.reserve(node.enumValues.size());

        for (auto& name: node.enumValues)
            names.emplace_back(name);

        obj["x-enumNames"] = JSON {std::move(names)};
    }

    JSON integerEnumBody(const TypeNode& node) const
    {
        auto out = makeObject();
        auto& obj = out.asObject();
        obj["type"] = makeString("integer");

        if (!node.enumNumbers.empty())
        {
            auto values = Json::Array {};
            values.reserve(node.enumNumbers.size());

            for (auto number: node.enumNumbers)
                values.emplace_back(static_cast<double>(number));

            obj["enum"] = JSON {std::move(values)};
            writeEnumNames(node, obj);
        }

        return out;
    }

    JSON enumBody(const TypeNode& node) const
    {
        if (node.enumIsInteger)
            return integerEnumBody(node);

        auto out = makeObject();
        auto& obj = out.asObject();
        obj["type"] = makeString("string");

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

    JSON arrayBody(const TypeNode& node, const std::string& path) const
    {
        auto out = makeObject();
        auto& obj = out.asObject();

        obj["type"] = makeString("array");
        obj["items"] = slot(*node.inner, path + "[]");

        if (node.minItems)
            obj["minItems"] = JSON {static_cast<double>(*node.minItems)};
        if (node.maxItems)
            obj["maxItems"] = JSON {static_cast<double>(*node.maxItems)};

        return out;
    }

    JSON mapBody(const TypeNode& node, const std::string& path) const
    {
        if (dialect.closedObjects)
            throwUnsupported("a map", path);

        auto out = makeObject();
        auto& obj = out.asObject();

        obj["type"] = makeString("object");
        obj["additionalProperties"] = slot(*node.inner, path + "{}");

        return out;
    }

    JSON unionBody(const TypeNode& node, const std::string& path) const
    {
        if (dialect.unionStyle == Schema::Dialect::UnionStyle::AnyOf)
            return anyOfUnionBody(node, path);

        return oneOfUnionBody(node, path);
    }

    // ----- UnionStyle::OneOf ------------------------------------------

    // `{"properties": {"type": {"const": 2}}, "required": ["type"]}` —
    // the half of a union arm that pins the discriminator.
    static JSON tagConstraint(const TypeNode& node, const TypeNode::Variant& variant)
    {
        auto constant = makeObject();
        constant.asObject()["const"] =
            variant.tagIsString ? JSON {variant.tag}
                                : JSON {std::strtod(variant.tag.c_str(), nullptr)};

        auto props = makeObject();
        props.asObject()[node.tagKey] = std::move(constant);

        auto required = Json::Array {};
        required.emplace_back(node.tagKey);

        auto out = makeObject();
        auto& obj = out.asObject();
        obj["type"] = makeString("object");
        obj["properties"] = std::move(props);
        obj["required"] = JSON {std::move(required)};

        return out;
    }

    JSON oneOfArm(const TypeNode& node,
                  const TypeNode::Variant& variant,
                  const std::string& path) const
    {
        auto parts = Json::Array {};
        parts.emplace_back(tagConstraint(node, variant));
        parts.emplace_back(slot(*variant.type, path));

        auto out = makeObject();
        out.asObject()["allOf"] = JSON {std::move(parts)};

        return out;
    }

    // A discriminated union is a oneOf over "tag pinned to this
    // literal, and the alternative's own body". Plain keys the same
    // reflect() body wrote next to the discriminator apply to every
    // arm, so they join as an outer allOf.
    JSON oneOfUnionBody(const TypeNode& node, const std::string& path) const
    {
        auto arms = Json::Array {};
        for (auto& variant: node.variants)
            arms.emplace_back(oneOfArm(node, variant, path));

        auto oneOf = makeObject();
        oneOf.asObject()["oneOf"] = JSON {std::move(arms)};

        if (node.fields.empty())
            return oneOf;

        auto parts = Json::Array {};
        parts.emplace_back(objectBody(node, path));
        parts.emplace_back(std::move(oneOf));

        auto out = makeObject();
        out.asObject()["allOf"] = JSON {std::move(parts)};

        return out;
    }

    // ----- UnionStyle::AnyOf ------------------------------------------

    // The node holding a named type's fields. A recursive back edge is
    // a bare name reference, so its body is looked up among the
    // declarations collected for $defs.
    const TypeNode& bodyOf(const TypeNode& node) const
    {
        if (!node.fields.empty() || node.qualifiedName.empty())
            return node;

        auto it = declarations.find(node.qualifiedName);
        return it != declarations.end() ? *it->second : node;
    }

    // `{"type": "string", "enum": ["circle"]}` — the discriminator as a
    // single-value enum, which every dialect that lacks `const` accepts.
    static JSON tagSchema(const TypeNode::Variant& variant)
    {
        auto out = makeObject();
        auto& obj = out.asObject();
        auto values = Json::Array {};

        if (variant.tagIsString)
        {
            obj["type"] = makeString("string");
            values.emplace_back(variant.tag);
        }
        else if (auto whole = Detail::parseWholeInteger(variant.tag))
        {
            obj["type"] = makeString("integer");
            values.emplace_back(*whole);
        }
        else
        {
            obj["type"] = makeString("number");
            values.emplace_back(std::strtod(variant.tag.c_str(), nullptr));
        }

        obj["enum"] = JSON {std::move(values)};
        return out;
    }

    JSON flatArm(const TypeNode& node,
                 const TypeNode::Variant& variant,
                 const std::string& path) const
    {
        auto& body = bodyOf(*variant.type);

        if (body.shape != TypeNode::Shape::Object)
            throw std::invalid_argument("union arm '" + variant.tag + "' at '" + path
                                        + "' is not an object and cannot be "
                                          "flattened into an anyOf arm");

        auto props = Json::Object {};
        auto required = Json::Array {};

        addProperties(node.fields, path, props, required);
        addProperties(body.fields, path, props, required);

        props[node.tagKey] = tagSchema(variant);
        addRequired(required, node.tagKey);

        return objectFrom(std::move(props), std::move(required));
    }

    JSON anyOfUnionBody(const TypeNode& node, const std::string& path) const
    {
        auto arms = Json::Array {};
        for (auto& variant: node.variants)
            arms.emplace_back(flatArm(node, variant, path));

        auto out = makeObject();
        out.asObject()["anyOf"] = JSON {std::move(arms)};
        return out;
    }

    const Schema::Dialect& dialect;
    std::map<std::string, const TypeNode*> declarations;
};

// Walks every named type reachable from the roots and returns a $defs
// object. Mutates roots in place via prepareRoots — the per-node
// disambiguation pass rewrites typeName when distinct types share a
// short name.
JSON buildDefs(const Renderer& renderer, const Vector<const TypeNode*>& ordered)
{
    auto defs = makeObject();
    for (auto* node: ordered)
        defs.asObject()[node->typeName] = renderer.declaration(*node);

    return defs;
}

} // namespace

// Multi-root variant — used by the type-export runner to bundle every
// registered type into a single document. No type is privileged as
// "the schema"; consumers point at one with {"$ref": "#/$defs/X"}.
JSON formatJsonSchema(std::span<TypeNode> roots, const Schema::Dialect& dialect)
{
    auto ordered = TypeTree::prepareRoots(roots);
    auto renderer = Renderer {dialect, ordered};
    auto defs = buildDefs(renderer, ordered);

    auto out = makeObject();
    if (!defs.asObject().empty())
        out.asObject()["$defs"] = std::move(defs);

    return out;
}

// Single-root: render the root (a $ref for named types, an inline body
// for top-level vectors / primitives) and attach $defs alongside. Used
// by `schemaOf<T>()`.
JSON formatJsonSchema(TypeNode& root, const Schema::Dialect& dialect)
{
    // Build $defs first — prepareRoots rewrites typeName during this
    // pass, and we need the renamed names when we render the root.
    auto ordered = TypeTree::prepareRoots(std::span {&root, 1});
    auto renderer = Renderer {dialect, ordered};
    auto defs = buildDefs(renderer, ordered);

    auto rootPath = root.typeName.empty() ? std::string {"<root>"} : root.typeName;
    auto out = renderer.slot(root, rootPath);

    if (!defs.asObject().empty())
    {
        if (!out.isObject())
            out = makeObject();
        out.asObject()["$defs"] = std::move(defs);
    }

    return out;
}

} // namespace Miro
