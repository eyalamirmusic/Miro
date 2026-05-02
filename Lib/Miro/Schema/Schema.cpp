#include "Schema.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace Miro
{

SchemaReflector::SchemaReflector(Json::Value& nodeToUse)
    : node(nodeToUse)
{
}

bool SchemaReflector::isSaving() const
{
    return true;
}

bool SchemaReflector::isLoading() const
{
    return false;
}

bool SchemaReflector::isSchema() const
{
    return true;
}

void SchemaReflector::markNullable()
{
    nextNullable = true;
}

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

void SchemaReflector::applyNullable()
{
    if (!nextNullable)
        return;

    if (!node.isObject())
        node = Json::Value {Json::Object {}};

    node.asObject()["nullable"] = Json::Value {true};
    nextNullable = false;
}

void SchemaReflector::writePrimitive(std::string_view typeName)
{
    node = primitiveSchema(typeName);
    applyNullable();
}

void SchemaReflector::enterScope(Json::Value initialiser,
                                 Scope newScope,
                                 const ScopeBody& body)
{
    node = std::move(initialiser);
    applyNullable();
    scope = newScope;
    body(*this);
}

void SchemaReflector::visit(PrimitiveRef ref)
{
    std::visit(
        [this](auto* ptr)
        {
            using T = std::remove_pointer_t<decltype(ptr)>;
            if constexpr (std::same_as<T, bool>)
                writePrimitive("boolean");
            else if constexpr (std::same_as<T, std::string>)
                writePrimitive("string");
            else if constexpr (std::same_as<T, double>)
                writePrimitive("number");
            else
                writePrimitive("integer");
        },
        ref.data);
}

void SchemaReflector::asObject(ScopeBody body)
{
    enterScope(objectSchema(), Scope::Object, body);
}

void SchemaReflector::asArray(ScopeBody body)
{
    enterScope(arraySchema(), Scope::Array, body);
}

void SchemaReflector::asMap(ScopeBody body)
{
    enterScope(mapSchema(), Scope::Map, body);
}

void SchemaReflector::atKey(std::string_view key, ScopeBody body)
{
    if (scope == Scope::Map)
    {
        auto& childNode = node.asObject()["additionalProperties"];
        auto child = SchemaReflector {childNode};
        body(child);
        return;
    }

    auto& props = node.asObject()["properties"];
    if (!props.isObject())
        props = Json::Value {Json::Object {}};

    auto& childNode = props.asObject()[std::string {key}];
    auto child = SchemaReflector {childNode};
    body(child);
}

void SchemaReflector::atIndex(std::size_t, ScopeBody body)
{
    auto& childNode = node.asObject()["items"];
    auto child = SchemaReflector {childNode};
    body(child);
}

std::size_t SchemaReflector::arraySize() const
{
    return 1;
}

void SchemaReflector::resizeArray(std::size_t) {}

std::vector<std::string> SchemaReflector::mapKeys() const
{
    return {};
}

} // namespace Miro
