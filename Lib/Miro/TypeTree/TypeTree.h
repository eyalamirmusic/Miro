#pragma once

#include "../Reflection/ReflectDispatch.h"
#include "../Reflection/Reflector.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Miro::TypeTree
{

enum class PrimitiveKind
{
    Boolean,
    String,
    Number,
    Integer,
    Int64,
};

struct TypeNode
{
    // Nested so it doesn't shadow Miro::Shape on Reflector::Options.
    enum class Shape
    {
        Primitive,
        Object,
        Array,
        Map,
        Enum,
    };

    Shape shape = Shape::Primitive;
    bool optional = false;

    PrimitiveKind primitive = PrimitiveKind::String;

    std::string typeName;

    // Dedup key: keeps same-short-name types from different namespaces apart.
    std::string qualifiedName;

    struct Field
    {
        std::string name;
        OwningPointer<TypeNode> type;
    };
    Vector<Field> fields;

    // Array / Map only.
    OwningPointer<TypeNode> inner;

    Vector<std::string> enumValues;

    // Both hold N for fixed-size arrays; nullopt for resizable ones.
    std::optional<std::size_t> minItems;
    std::optional<std::size_t> maxItems;
};

class TypeReflector final : public Reflector
{
public:
    TypeReflector(TypeNode& nodeToUse,
                  Options optsToUse,
                  TypeReflector* parentToUse = nullptr);
    ~TypeReflector() override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;
    bool beginNamedType(TypeId id) override;
    void visitEnum(TypeId id, const Vector<std::string_view>& names) override;

    Reflector& atKey(std::string_view key, Options childOpts) override;
    Reflector& atIndex(std::size_t index, Options childOpts) override;

    void setArrayBounds(std::size_t min, std::size_t max) override;

private:
    TypeNode& node;
    OwningPointer<TypeReflector> currentChild;

    // nullptr at the root; beginNamedType walks this chain against
    // activeQualifiedName to turn a recursive descent into a name reference.
    TypeReflector* parent;

    std::string activeQualifiedName;

    Reflector& spawnChild(TypeNode& targetNode, Options childOpts);
};

template <typename T>
TypeNode buildTree()
{
    auto root = TypeNode {};
    auto opts = Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = TypeReflector {root, opts};
    auto value = T {};
    Detail::reflectValue(reflector, value);
    return root;
}

// Returns the named-type nodes in dependency order, rewriting colliding
// `typeName`s in place. Pointers stay valid as long as `roots` does.
Vector<const TypeNode*> prepareRoots(std::span<TypeNode> roots);

} // namespace Miro::TypeTree
