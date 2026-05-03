#pragma once

#include "../Reflection/ReflectDispatch.h"
#include "../Reflection/Reflector.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Miro
{

// Walks a default-constructed value's reflect() method and produces a
// JSON-Schema-shaped description of its structure. Like JsonReflector,
// each instance points at one slot — the shape (object/array/map/
// primitive) and the nullable bit are committed at construction time
// from Options. atKey/atIndex hand back a child SchemaReflector
// positioned at the appropriate sub-slot.
class SchemaReflector final : public Reflector
{
public:
    SchemaReflector(Json::Value& nodeToUse, Options optsToUse);
    ~SchemaReflector() override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;
    void visitEnum(TypeId id, const std::vector<std::string_view>& names) override;

    Reflector& atKey(std::string_view key, Options childOpts) override;
    Reflector& atIndex(std::size_t index, Options childOpts) override;

    void setArrayBounds(std::size_t min, std::size_t max) override;

private:
    Json::Value& node;
    std::unique_ptr<SchemaReflector> currentChild;

    void commitShape();
    void applyNullable();
    void appendRequired(std::string_view key);
    Reflector& spawnChild(Json::Value& targetNode, Options childOpts);

    static Json::Value primitiveSchema(std::string_view typeName);
    static Json::Value objectSchema();
    static Json::Value arraySchema();
    static Json::Value mapSchema();
    static Json::Value enumSchema(const std::vector<std::string_view>& names);
};

template <typename T>
Json::Value schemaOf()
{
    auto json = Json::Value {Json::Object {}};
    auto opts = Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = SchemaReflector {json, opts};
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
