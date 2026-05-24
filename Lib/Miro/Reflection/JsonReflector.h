#pragma once

#include "Reflector.h"

namespace Miro
{

// A JsonReflector represents a single JSON slot. The shape
// (object/array/map/primitive) is fixed at construction via Options
// and committed eagerly when saving — atKey/atIndex spawn children
// that commit their own shape the same way. The child is owned by
// the parent and lives until the next atKey/atIndex call (or until
// the parent is destroyed).
class JsonReflector final : public Reflector
{
public:
    JsonReflector(JSON& slotToUse, Options optsToUse);
    ~JsonReflector() override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;

    Reflector& atKey(std::string_view key, Options childOpts) override;
    Reflector& atIndex(std::size_t index, Options childOpts) override;

    std::size_t arraySize() const override;
    void resizeArray(std::size_t newSize) override;
    Vector<std::string> mapKeys() const override;

    void requirePolymorphicSupport(std::string_view) override {}

private:
    JSON& slot;
    bool absent = false;

    // Sentinel used as the target slot when loading and the requested
    // key/index isn't present. Operations on a child pointing at it
    // become no-ops, matching the prior "skip body" semantics.
    JSON missingSlot;

    OwningPointer<JsonReflector> currentChild;

    JsonReflector(JSON& slotToUse, Options optsToUse, bool absentToUse);

    void commitShape();

    void savePrimitive(PrimitiveRef ref);
    void loadPrimitive(PrimitiveRef ref);

    Reflector& atKeyForSave(std::string_view key, Options childOpts);
    Reflector& atKeyForLoad(std::string_view key, Options childOpts);

    Reflector& atIndexForSave(std::size_t index, Options childOpts);
    Reflector& atIndexForLoad(std::size_t index, Options childOpts);

    Reflector& spawnChild(JSON& targetSlot, Options childOpts, bool absentToUse);
    Reflector& spawnMissingChild(Options childOpts);
};

} // namespace Miro
