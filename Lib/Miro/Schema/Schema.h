#pragma once

#include "../Reflection/ReflectDispatch.h"
#include "../Reflection/Reflector.h"

#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace Miro
{

namespace Detail
{

// Shared state threaded through the SchemaReflector chain. Owned by
// the root reflector; child reflectors hold a non-owning pointer. The
// $defs map gathers every named type's body so each type appears once
// regardless of how many slots reference it, and recursive references
// terminate naturally on the second visit (the type is already in
// $defs, so we just emit a $ref and skip the body).
struct SchemaContext
{
    Json::Value& defs;
    std::set<std::string> known;
};

} // namespace Detail

// Walks a default-constructed value's reflect() method and produces a
// JSON-Schema-shaped description of its structure. Each instance points
// at one slot. When the dispatcher announces a named user type via
// beginNamedType, the reflector hoists that type's body into $defs and
// stamps a $ref into the slot — recursive references terminate
// because subsequent visits hit a known $defs entry.
class SchemaReflector final : public Reflector
{
public:
    SchemaReflector(Json::Value& nodeToUse,
                    Options optsToUse,
                    Detail::SchemaContext* contextToUse = nullptr);
    ~SchemaReflector() override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;
    bool beginNamedType(TypeId id) override;
    void visitEnum(TypeId id, const std::vector<std::string_view>& names) override;

    Reflector& atKey(std::string_view key, Options childOpts) override;
    Reflector& atIndex(std::size_t index, Options childOpts) override;

    void setArrayBounds(std::size_t min, std::size_t max) override;

private:
    // Pointer (rather than reference) so beginNamedType can repoint us
    // at the freshly-allocated $defs entry when we hoist a named type.
    Json::Value* node;
    Detail::SchemaContext* context;
    std::unique_ptr<SchemaReflector> currentChild;

    void commitShape();
    void applyNullable();
    void appendRequired(std::string_view key);
    void replaceWithRef(const std::string& defsName);
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
    auto root = Json::Value {Json::Object {}};
    auto defs = Json::Value {Json::Object {}};
    auto context = Detail::SchemaContext {defs, {}};

    auto opts = Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = SchemaReflector {root, opts, &context};
    auto value = T {};
    Detail::reflectValue(reflector, value);

    // Merge the collected $defs into the root unless empty (e.g. a
    // top-level vector with primitive elements has nothing to hoist).
    if (!defs.asObject().empty())
    {
        if (!root.isObject())
            root = Json::Value {Json::Object {}};
        root.asObject()["$defs"] = std::move(defs);
    }

    return root;
}

template <typename T>
std::string schemaOfAsString(int indent = 2)
{
    return Json::print(schemaOf<T>(), indent);
}

} // namespace Miro
