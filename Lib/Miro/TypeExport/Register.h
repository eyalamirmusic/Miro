#pragma once

#include "../Reflection/ReflectDispatch.h"
#include "../Reflection/Reflector.h"
#include "../Reflection/TypeName.h"

#include <memory>
#include <string>
#include <vector>

namespace Miro::TypeExport
{

// Type-erased description of one registered type. The runner asks each
// entry for its top-level Options (so it can build a format-specific
// reflector) and then has the entry drive reflect() of a default-
// constructed value of T into that reflector. The entry itself knows
// nothing about formats — adding a new exporter is purely a runner-side
// change.
struct TypeEntry
{
    virtual ~TypeEntry() = default;

    std::string typeName;

    virtual Options topLevelOptions(Mode mode, bool schema) const = 0;
    virtual void reflect(Reflector& ref) const = 0;
};

namespace Detail
{

template <typename T>
struct TypedEntry final : TypeEntry
{
    Options topLevelOptions(Mode mode, bool schema) const override
    {
        return Miro::Detail::topLevelOptions<T>(mode, schema);
    }

    void reflect(Reflector& ref) const override
    {
        auto value = T {};
        Miro::Detail::reflectValue(ref, value);
    }
};

// Process-wide registry, populated by static initializers from
// MIRO_EXPORT_TYPES(...) at program start. The runner walks this in main().
std::vector<std::unique_ptr<TypeEntry>>& registry();

template <typename T>
inline void registerOne()
{
    auto entry = std::make_unique<TypedEntry<T>>();
    entry->typeName = std::string {Miro::Detail::typeNameOf<T>()};
    registry().push_back(std::move(entry));
}

// Anchors a global per macro invocation; its constructor populates the
// registry for the whole pack of types.
template <typename... Ts>
struct MultiRegistrar
{
    MultiRegistrar() { (registerOne<Ts>(), ...); }
};

} // namespace Detail
} // namespace Miro::TypeExport

#define MIRO_EXPORT_TYPES_CAT2(a, b) a##b
#define MIRO_EXPORT_TYPES_CAT(a, b) MIRO_EXPORT_TYPES_CAT2(a, b)

// Registers one or more types with the type-export runner. Use at
// namespace scope from a .cpp file linked into a target passed to
// miro_add_type_export() — the CMake helper links such targets with
// WHOLE_ARCHIVE so these static initializers survive.
//
// Usage: MIRO_EXPORT_TYPES(User, Address, Role)
//
// Uniqueness of the generated variable name comes from __LINE__, which
// is fully standard (unlike __COUNTER__). The only collision case is
// two MIRO_EXPORT_TYPES on the same source line, which is unrealistic.
#define MIRO_EXPORT_TYPES(...)                                                      \
    namespace                                                                       \
    {                                                                               \
    [[maybe_unused]] const ::Miro::TypeExport::Detail::MultiRegistrar<__VA_ARGS__>  \
        MIRO_EXPORT_TYPES_CAT(miroTypeExportMultiRegistrar_, __LINE__) {};          \
    }

// Single-type alias retained for clarity at call sites. Forwards to the
// variadic form, so both macros share one fan-out path.
#define MIRO_EXPORT_TYPE(T) MIRO_EXPORT_TYPES(T)
