#pragma once

#include "../Containers.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Miro
{
class Reflector;

struct Property
{
    template <typename T>
    void operator()(T& value);

    Reflector& reflector;
    std::string_view key;
};

struct Element
{
    template <typename T>
    void operator()(T& value);

    Reflector& reflector;
    std::size_t index;
};

enum class Mode
{
    Save,
    Load
};

enum class Shape
{
    Primitive,
    Object,
    Array,
    Map
};

enum class ValueKind
{
    Absent,
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct Options
{
    Mode mode = Mode::Save;
    Shape shape = Shape::Primitive;
    bool nullable = false;
    bool schema = false;
};

// qualifiedName is the dedup key: short names collide across namespaces.
struct TypeId
{
    std::string_view shortName;
    std::string_view qualifiedName;
};

struct PrimitiveRef
{
    using Variant = std::variant<bool*, int*, double*, std::string*, std::int64_t*>;

    PrimitiveRef(bool& value)
        : data(&value)
    {
    }
    PrimitiveRef(int& value)
        : data(&value)
    {
    }
    PrimitiveRef(double& value)
        : data(&value)
    {
    }
    PrimitiveRef(std::string& value)
        : data(&value)
    {
    }
    PrimitiveRef(std::int64_t& value)
        : data(&value)
    {
    }

    Variant data;
};

// One Reflector is one slot in the tree, configured once at construction;
// recursion happens through the child reflectors atKey/atIndex return.
class Reflector
{
public:
    explicit Reflector(Options optsToUse)
        : opts(optsToUse)
    {
    }

    virtual ~Reflector() = default;

    Property operator[](std::string_view key);
    Element operator[](std::size_t index);

    const Options& options() const { return opts; }
    Mode mode() const { return opts.mode; }
    Shape shape() const { return opts.shape; }
    bool isSaving() const { return opts.mode == Mode::Save; }
    bool isLoading() const { return opts.mode == Mode::Load; }
    bool isSchema() const { return opts.schema; }
    bool isNullable() const { return opts.nullable; }

    virtual void visit(PrimitiveRef ref) = 0;
    virtual void writeNull() = 0;
    virtual ValueKind kind() const = 0;

    // Called just before a reflectable type's own reflect() body runs.
    // Returning false skips that body — how a reflector breaks recursion
    // or emits a reference to the type instead of inlining it.
    virtual bool beginNamedType(TypeId /*id*/) { return true; }

    // Only called in schema mode; the default degrades to a string slot.
    virtual void visitEnum(TypeId /*id*/, const Vector<std::string_view>& /*names*/)
    {
        auto placeholder = std::string {"enum"};
        visit(placeholder);
    }

    // The child is owned by this reflector and stays valid only until the
    // next atKey/atIndex call on it, or until this reflector dies.
    virtual Reflector& atKey(std::string_view key, Options childOpts) = 0;
    virtual Reflector& atIndex(std::size_t index, Options childOpts) = 0;

    virtual std::size_t arraySize() const { return 0; }
    virtual void resizeArray(std::size_t /*newSize*/) {}
    virtual Vector<std::string> mapKeys() const { return {}; }

    // Schema-only hint; a fixed-size array passes N as both min and max.
    virtual void setArrayBounds(std::size_t /*min*/, std::size_t /*max*/) {}

    // Data-walking reflectors override this with a no-op. The default
    // throws because a schema walked from a default-constructed
    // polymorphic value would describe only one arbitrary alternative.
    virtual void requirePolymorphicSupport(std::string_view context);

protected:
    Options opts;
};

} // namespace Miro
