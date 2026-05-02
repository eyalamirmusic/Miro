#pragma once

#include "Reflector.h"

namespace Miro
{

// A JsonReflector represents a single Json::Value slot. Recursing into a
// nested object/array constructs a child JsonReflector pointing at the
// child slot — no shared stack between parent and child.
class JsonReflector final : public Reflector
{
public:
    enum class Mode
    {
        Save,
        Load
    };

    JsonReflector(Json::Value& slotToUse, Mode modeToUse);

    bool isSaving() const override;
    bool isLoading() const override;
    bool isSchema() const override;

    void visit(PrimitiveRef ref) override;
    void writeNull() override;
    ValueKind kind() const override;

    void asObject(ScopeBody body) override;
    void asArray(ScopeBody body) override;
    void asMap(ScopeBody body) override;

    void atKey(std::string_view key, ScopeBody body) override;
    void atIndex(std::size_t index, ScopeBody body) override;

    std::size_t arraySize() const override;
    void resizeArray(std::size_t newSize) override;
    std::vector<std::string> mapKeys() const override;

private:
    Json::Value& slot;
    Mode mode;
};

} // namespace Miro
