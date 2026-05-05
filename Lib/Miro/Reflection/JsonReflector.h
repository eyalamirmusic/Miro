#pragma once

#include "Reflector.h"

#include <memory>

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
    std::vector<std::string> mapKeys() const override;

private:
    JSON& slot;
    bool absent = false;

    // Sentinel used as the target slot when loading and the requested
    // key/index isn't present. Operations on a child pointing at it
    // become no-ops, matching the prior "skip body" semantics.
    JSON missingSlot;

    std::unique_ptr<JsonReflector> currentChild;

    JsonReflector(JSON& slotToUse, Options optsToUse, bool absentToUse);

    void commitShape();
    Reflector&
        spawnChild(JSON& targetSlot, Options childOpts, bool absentToUse);
};

} // namespace Miro
