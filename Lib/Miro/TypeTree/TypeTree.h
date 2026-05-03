#pragma once

#include "../Reflection/ReflectDispatch.h"
#include "../Reflection/Reflector.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// A format-neutral description of a reflected C++ type. The TypeReflector
// walks any MIRO_REFLECT-enabled value and builds a TypeNode tree;
// downstream renderers (Zod, plain TypeScript, JSON Schema, future
// formats) consume the same tree, so the structural reflection logic
// is written once and the per-format work shrinks to "walk this tree
// and emit text/JSON".

namespace Miro::TypeTree
{

// Primitive flavour. Renderers map this to format-specific spellings
// (e.g. Boolean → "z.boolean()" / "boolean" / "{type:'boolean'}").
enum class PrimitiveKind
{
    Boolean,
    String,
    Number,
    Integer,
};

struct TypeNode
{
    // Nested so it doesn't shadow Miro::Shape (which lives on the
    // parent Reflector's Options). The IR superset adds an Enum case.
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

    // Primitive only.
    PrimitiveKind primitive = PrimitiveKind::String;

    // Object / Enum only: the display name emitted in generated source.
    // Defaults to the unqualified C++ type name; on collision the
    // disambiguation pass rewrites it to a sanitized qualified form.
    // Empty if anonymous (only meaningful for objects — enums always
    // come with a name from the dispatcher).
    std::string typeName;

    // Object / Enum only: the raw qualified C++ name from the compiler
    // (e.g. "Ns::Inner::Foo"). Stable per type and used as the dedup
    // key so two types in different namespaces with the same short
    // name don't silently collapse into one declaration.
    std::string qualifiedName;

    // Object only: ordered fields.
    struct Field
    {
        std::string name;
        std::unique_ptr<TypeNode> type;
    };
    std::vector<Field> fields;

    // Array / Map only: the inner element type.
    std::unique_ptr<TypeNode> inner;

    // Enum only: ordered enumerator names.
    std::vector<std::string> enumValues;

    // Array only, and only when fixed-size (std::array<T, N>): both
    // bounds get N. nullopt for std::vector. Schema renderer uses these
    // to emit minItems/maxItems.
    std::optional<std::size_t> minItems;
    std::optional<std::size_t> maxItems;
};

// Walks a default-constructed value of T via reflection and produces
// the structural TypeNode tree. Used by both the TypeScript and Schema
// renderers.
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
    void visitEnum(TypeId id, const std::vector<std::string_view>& names) override;

    Reflector& atKey(std::string_view key, Options childOpts) override;
    Reflector& atIndex(std::size_t index, Options childOpts) override;

    void setArrayBounds(std::size_t min, std::size_t max) override;

private:
    TypeNode& node;
    std::unique_ptr<TypeReflector> currentChild;

    // Parent in the spawn chain; nullptr at the root. Used by
    // beginNamedType to detect a recursive descent into the same C++
    // type — the slot becomes a name reference and the body is skipped.
    TypeReflector* parent;

    // Qualified name being walked at this slot. Set in beginNamedType
    // when the body is going to run; descendants check the parent chain
    // against this to detect cycles.
    std::string activeQualifiedName;

    Reflector& spawnChild(TypeNode& targetNode, Options childOpts);
};

template <typename T>
TypeNode buildTree()
{
    auto root = TypeNode {};
    auto opts = Miro::Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = TypeReflector {root, opts};
    auto value = T {};
    Miro::Detail::reflectValue(reflector, value);
    return root;
}

// Post-walk pass shared by every renderer: collects the named-type
// nodes reachable from any root in dependency order, and resolves
// display-name collisions by rewriting `typeName` on each affected
// TypeNode in place. The returned pointers reference nodes inside
// `roots` (or their subtrees) — valid for as long as `roots` is alive.
//
// Roots are passed mutably because the rewrite happens in place; if a
// caller doesn't want their trees touched, hand over a copy.
std::vector<const TypeNode*> prepareRoots(std::span<TypeNode> roots);

} // namespace Miro::TypeTree
