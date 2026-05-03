#pragma once

#include "../Reflection/ReflectDispatch.h"
#include "../Reflection/Reflector.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Miro::TypeScript
{

namespace Detail
{

// Internal representation built up during reflection. Each TsNode
// describes one slot's shape; named object slots also remember their
// C++ type name so they can be hoisted into a top-level z.object()
// declaration during emission.
struct TsNode
{
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

    // Primitive only: the Zod expression (e.g. "z.string()").
    std::string primitive;

    // Object / Enum only: the (unqualified) C++ type name. Empty if
    // anonymous (only meaningful for objects — enums always come with a
    // name from the dispatcher).
    std::string typeName;

    // Object only: ordered fields.
    struct Field
    {
        std::string name;
        std::unique_ptr<TsNode> type;
    };
    std::vector<Field> fields;

    // Array / Map only: the inner element type.
    std::unique_ptr<TsNode> inner;

    // Enum only: ordered enumerator names.
    std::vector<std::string> enumValues;
};

} // namespace Detail

// Walks a default-constructed value of T via reflection, builds a
// TsNode tree, and emits a self-contained TypeScript module that
// declares Zod schemas for T and every named user-defined struct it
// transitively reaches. Anonymous nested shapes are inlined.
class TypeScriptReflector final : public Reflector
{
public:
    TypeScriptReflector(Detail::TsNode& nodeToUse, Options optsToUse);
    ~TypeScriptReflector() override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;
    void beginNamedType(std::string_view typeName) override;
    void visitEnum(std::string_view typeName,
                   const std::vector<std::string_view>& names) override;

    Reflector& atKey(std::string_view key, Options childOpts) override;
    Reflector& atIndex(std::size_t index, Options childOpts) override;

private:
    Detail::TsNode& node;
    std::unique_ptr<TypeScriptReflector> currentChild;

    Reflector& spawnChild(Detail::TsNode& targetNode, Options childOpts);
};

// Walks a default-constructed value of T through the reflection
// machinery and returns the resulting TsNode tree. Shared root for the
// Zod and plain-TS emitters.
template <typename T>
Detail::TsNode buildTree()
{
    auto root = Detail::TsNode {};
    auto opts = Miro::Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = TypeScriptReflector {root, opts};
    auto value = T {};
    Miro::Detail::reflectValue(reflector, value);
    return root;
}

std::string formatZodModule(const Detail::TsNode& root);
std::string formatTypesModule(const Detail::TsNode& root);

// Multi-root variants — emit one self-contained module that declares
// every named (object or enum) type reachable from any of the roots,
// deduped by name. Used by the type-export runner to bundle all
// registered types into a single .zod.ts / .ts file. The single-root
// versions above add a default export for anonymous roots; the bundled
// versions skip that because a module only allows one default export.
std::string formatZodModule(std::span<const Detail::TsNode> roots);
std::string formatTypesModule(std::span<const Detail::TsNode> roots);

// Public entry points.
template <typename T>
std::string toZod()
{
    return formatZodModule(buildTree<T>());
}

template <typename T>
std::string toTypes()
{
    return formatTypesModule(buildTree<T>());
}

} // namespace Miro::TypeScript
