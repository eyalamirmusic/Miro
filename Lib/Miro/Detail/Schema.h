#pragma once

#include "ReflectDispatch.h"
#include "Reflector.h"

#include <string>
#include <string_view>
#include <vector>

namespace Miro
{

// Walks a default-constructed value's reflect() method and produces a
// JSON-Schema-shaped description of its structure. Like JsonReflector,
// each instance points at one slot — recursing into nested types
// constructs a child SchemaReflector for the appropriate sub-slot.
class SchemaReflector final : public Reflector
{
public:
    explicit SchemaReflector(Json::Value& nodeToUse);

    bool isSaving() const override;
    bool isLoading() const override;
    bool isSchema() const override;

    void markNullable() override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;

    void asObject(ScopeBody body) override;
    void asArray(ScopeBody body) override;
    void asMap(ScopeBody body) override;

    void atKey(std::string_view key, ScopeBody body) override;
    void atIndex(std::size_t index, ScopeBody body) override;

    std::size_t arraySize() const override;
    void resizeArray(std::size_t newSize) override;
    std::vector<std::string> mapKeys() const override;

private:
    enum class Scope
    {
        Unknown,
        Object,
        Array,
        Map
    };

    Json::Value& node;
    Scope scope = Scope::Unknown;
    bool nextNullable = false;

    void writePrimitive(std::string_view typeName);
    void enterScope(Json::Value initialiser, Scope newScope, const ScopeBody& body);
    void applyNullable();

    static Json::Value primitiveSchema(std::string_view typeName);
    static Json::Value objectSchema();
    static Json::Value arraySchema();
    static Json::Value mapSchema();
};

template <typename T>
Json::Value schemaOf()
{
    auto json = Json::Value {Json::Object {}};
    auto reflector = SchemaReflector {json};
    auto value = T {};
    Detail::reflectValue(reflector, value);
    return json;
}

template <typename T>
std::string schemaOfAsString(int indent = 2)
{
    return Json::print(schemaOf<T>(), indent);
}

} // namespace Miro
