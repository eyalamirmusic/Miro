#pragma once

#include "../XML/Xml.h"
#include "Reflector.h"

#include <string>
#include <variant>

namespace Miro
{

// An XmlReflector targets one of four "slot" kinds, picked at spawn
// time from the parent's atKey/atIndex call and the child's
// childOpts.shape:
//
//   * ElementSlot   — a real <element> node. Created by atKey when
//                     the child is Object/Map-shaped, by atIndex
//                     under an ArraySlot, or supplied at the root.
//                     visit() reads/writes the node's text content.
//   * AttributeSlot — an attribute on a parent node. Created by atKey
//                     when the child is Primitive-shaped. visit()
//                     reads/writes the parent's attributes[name].
//   * ArraySlot     — "repeated siblings named X under parent". Created
//                     by atKey when the child is Array-shaped (or by
//                     atIndex when the array element type is itself
//                     array-shaped, using "item" as the fallback name).
//                     atIndex(i) targets the i-th such sibling.
//   * MissingSlot   — load-time sentinel for an absent attribute /
//                     child / array sibling. All operations are no-ops.
//
// Like JsonReflector, the parent owns its currently-active child via
// OwningPointer; the child is destroyed when the parent spawns a new
// one (or when the parent is destroyed).
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
