#pragma once

#include "../XML/Xml.h"
#include "Reflector.h"

#include <string>
#include <variant>

namespace Miro
{

// The mapping: primitive fields become attributes, object and map fields
// become child elements, and array fields become repeated siblings named
// after the field. MissingSlot is the load-time no-op for absent nodes.
class XmlReflector final : public Reflector
{
public:
    XmlReflector(Xml::Node& rootToUse, Options optsToUse);
    ~XmlReflector() override;

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
    struct ElementSlot
    {
        Xml::Node* node;
    };

    struct AttributeSlot
    {
        Xml::Node* parent;
        std::string name;
    };

    struct ArraySlot
    {
        Xml::Node* parent;
        std::string elementName;
    };

    struct MissingSlot
    {
    };

    using Slot = std::variant<ElementSlot, AttributeSlot, ArraySlot, MissingSlot>;

    Slot slot;
    OwningPointer<XmlReflector> currentChild;

    XmlReflector(Slot slotToUse, Options optsToUse);

    Reflector& spawnElement(Xml::Node& node, Options childOpts);
    Reflector&
        spawnAttribute(Xml::Node& parent, std::string name, Options childOpts);
    Reflector& spawnArray(Xml::Node& parent, std::string name, Options childOpts);
    Reflector& spawnMissing(Options childOpts);

    Reflector&
        atKeyOnElement(Xml::Node& parent, std::string_view key, Options childOpts);
    Reflector& atIndexOnArray(Xml::Node& parent,
                              const std::string& elementName,
                              std::size_t index,
                              Options childOpts);
};

} // namespace Miro
