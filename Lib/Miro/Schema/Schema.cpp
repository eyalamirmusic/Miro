#include "Schema.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace Miro
{

SchemaReflector::SchemaReflector(Json::Value& nodeToUse, Options optsToUse)
    : Reflector(optsToUse)
    , node(nodeToUse)
{
    commitShape();
}

SchemaReflector::~SchemaReflector() = default;

ValueKind SchemaReflector::kind() const
{
    return ValueKind::Absent;
}

void SchemaReflector::writeNull() {}

Json::Value SchemaReflector::primitiveSchema(std::string_view typeName)
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {typeName}};
    return value;
}

Json::Value SchemaReflector::objectSchema()
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"object"}};
    value.asObject()["properties"] = Json::Value {Json::Object {}};
    return value;
}

Json::Value SchemaReflector::arraySchema()
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"array"}};
    value.asObject()["items"] = Json::Value {Json::Object {}};
    return value;
}

Json::Value SchemaReflector::mapSchema()
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"object"}};
    value.asObject()["additionalProperties"] = Json::Value {Json::Object {}};
    return value;
}

Json::Value SchemaReflector::enumSchema(const std::vector<std::string_view>& names)
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"string"}};

    if (!names.empty())
    {
        auto arr = Json::Array {};
        arr.reserve(names.size());

        for (auto& name: names)
            arr.emplace_back(std::string {name});

        value.asObject()["enum"] = Json::Value {std::move(arr)};
    }

    return value;
}

void SchemaReflector::commitShape()
{
    switch (opts.shape)
    {
        case Shape::Primitive:
            // visit() will write the {type:...} object once it knows
            // the concrete primitive type.
            break;
        case Shape::Object:
            node = objectSchema();
            break;
        case Shape::Array:
            node = arraySchema();
            break;
        case Shape::Map:
            node = mapSchema();
            break;
    }

    if (opts.shape != Shape::Primitive)
        applyNullable();
}

void SchemaReflector::applyNullable()
{
    if (!opts.nullable)
        return;

    if (!node.isObject())
        node = Json::Value {Json::Object {}};

    node.asObject()["nullable"] = Json::Value {true};
}

void SchemaReflector::visit(PrimitiveRef ref)
{
    std::visit(
        [this](auto* ptr)
        {
            using T = std::remove_pointer_t<decltype(ptr)>;
            auto typeName = std::string_view {};
            if constexpr (std::same_as<T, bool>)
                typeName = "boolean";
            else if constexpr (std::same_as<T, std::string>)
                typeName = "string";
            else if constexpr (std::same_as<T, double>)
                typeName = "number";
            else
                typeName = "integer";

            node = primitiveSchema(typeName);
            applyNullable();
        },
        ref.data);
}

void SchemaReflector::visitEnum(std::string_view,
                                const std::vector<std::string_view>& names)
{
    node = enumSchema(names);
    applyNullable();
}

Reflector& SchemaReflector::spawnChild(Json::Value& targetNode, Options childOpts)
{
    // Destroy previous child first; see JsonReflector::spawnChild.
    currentChild.reset();
    currentChild = std::make_unique<SchemaReflector>(targetNode, childOpts);
    return *currentChild;
}

Reflector& SchemaReflector::atKey(std::string_view key, Options childOpts)
{
    if (opts.shape == Shape::Map)
        return spawnChild(node.asObject()["additionalProperties"], childOpts);

    auto& props = node.asObject()["properties"];
    if (!props.isObject())
        props = Json::Value {Json::Object {}};

    if (!childOpts.nullable)
        appendRequired(key);

    return spawnChild(props.asObject()[std::string {key}], childOpts);
}

Reflector& SchemaReflector::atIndex(std::size_t, Options childOpts)
{
    return spawnChild(node.asObject()["items"], childOpts);
}

void SchemaReflector::setArrayBounds(std::size_t min, std::size_t max)
{
    if (!node.isObject())
        return;

    auto& obj = node.asObject();
    obj["minItems"] = Json::Value {static_cast<double>(min)};
    obj["maxItems"] = Json::Value {static_cast<double>(max)};
}

void SchemaReflector::appendRequired(std::string_view key)
{
    auto& obj = node.asObject();
    auto it = obj.find("required");

    if (it == obj.end())
    {
        auto arr = Json::Array {};
        arr.emplace_back(std::string {key});
        obj["required"] = Json::Value {std::move(arr)};
        return;
    }

    std::get<Json::Array>(it->second.data).emplace_back(std::string {key});
}

} // namespace Miro
