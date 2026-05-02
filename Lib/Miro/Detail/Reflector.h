#pragma once

#include "Json.h"

#include <cstddef>
#include <cstdint>
#include <functional>
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

// First-class primitive handle. Constructs implicitly from any built-in
// primitive — `ref.visit(myInt)` Just Works. To support a new primitive
// type, add another constructor + variant alternative below (or provide
// a free `makePrimitiveRef(T&)` overload and a templated constructor).
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

// A Reflector represents exactly one slot in a tree. It carries no path
// state and no scope stack. Recursion is expressed by the parent
// constructing a stack-allocated child reflector positioned at a child
// slot via atKey() / atIndex().
class Reflector
{
public:
    virtual ~Reflector() = default;

    Property operator[](std::string_view key);
    Element operator[](std::size_t index);

    virtual bool isSaving() const = 0;
    virtual bool isLoading() const = 0;
    virtual bool isSchema() const = 0;

    // Decorator hint — no-op default. SchemaReflector overrides to mark
    // the next reflected slot as nullable.
    virtual void markNullable() {}

    // Operate on this slot directly.
    virtual void visit(PrimitiveRef ref) = 0;
    virtual void writeNull() = 0;
    virtual ValueKind kind() const = 0;

    // Commit this slot's shape and run body. body's reflector argument is
    // *this — now in the matching mode.
    using ScopeBody = std::function<void(Reflector&)>;
    virtual void asObject(ScopeBody body) = 0;
    virtual void asArray(ScopeBody body) = 0;
    virtual void asMap(ScopeBody body) = 0;

    // Spawn a child reflector for a slot inside this one. The child is
    // stack-allocated by the implementation and lives only during body's
    // execution. Valid inside an as*() body of the matching shape.
    virtual void atKey(std::string_view key, ScopeBody body) = 0;
    virtual void atIndex(std::size_t index, ScopeBody body) = 0;

    // Iteration helpers — only meaningful inside an asArray / asMap body.
    virtual std::size_t arraySize() const = 0;
    virtual void resizeArray(std::size_t newSize) = 0;
    virtual std::vector<std::string> mapKeys() const = 0;
};

} // namespace Miro
