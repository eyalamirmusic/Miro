#pragma once

// Concrete ApiReflector for the describe path: walks an API's
// reflect() body and records each command / event into local lists.
// Used in tests to assert what reflect() declares; also the basis
// the eventual codegenMain<T> builds on (it forwards the recorded
// metadata into the format pipeline instead of process-wide
// registries).
//
// Header-only because it has no Bridge dependency — describe mode
// records metadata, never installs handlers or attaches listeners.

#include "../TypeTree/TypeTree.h"
#include "ApiReflector.h"

#include <ea_data_structures/Structures/Vector.h>

#include <string>

namespace Miro::Detail
{

class DescribeReflector : public ApiReflector
{
public:
    struct CommandRecord
    {
        std::string name;
        bool hasReq = false;
        bool hasRes = false;
        std::string reqTypeName;
        std::string reqQualifiedName;
        std::string resTypeName;
        std::string resQualifiedName;
    };

    struct EventRecord
    {
        std::string name;
        std::string payloadTypeName;
        std::string payloadQualifiedName;
        std::function<JSON()> defaultPayloadJson;

        bool isKeyed = false;
        std::string collectionField;
        std::string keyField;
    };

    EA::Vector<CommandRecord> commands;
    EA::Vector<EventRecord> events;

    // Structural TypeNodes for every Req / Res / event payload seen on
    // this walk, in declaration order. Duplicates are deliberately kept
    // — TypeTree::prepareRoots dedupes by qualifiedName at format time,
    // so the same tree built twice is harmless and avoids a hash-set
    // dependency here.
    EA::Vector<TypeTree::TypeNode> typeRoots;

protected:
    void commandImpl(const CommandDescriptor& d) override
    {
        auto record = CommandRecord {};
        record.name = std::string {d.name};
        record.hasReq = d.hasReq;
        record.hasRes = d.hasRes;
        record.reqTypeName = std::string {d.reqTypeName};
        record.reqQualifiedName = std::string {d.reqQualifiedName};
        record.resTypeName = std::string {d.resTypeName};
        record.resQualifiedName = std::string {d.resQualifiedName};
        commands.add(std::move(record));

        if (d.buildReqTree)
            d.buildReqTree(typeRoots.emplace_back());
        if (d.buildResTree)
            d.buildResTree(typeRoots.emplace_back());
    }

    void eventImpl(const EventDescriptor& d) override
    {
        auto record = EventRecord {};
        record.name = std::string {d.name};
        record.payloadTypeName = std::string {d.payloadTypeName};
        record.payloadQualifiedName = std::string {d.payloadQualifiedName};
        record.defaultPayloadJson = d.defaultPayloadJson;
        record.isKeyed = d.isKeyed;
        record.collectionField = std::string {d.collectionField};
        record.keyField = std::string {d.keyField};
        events.add(std::move(record));

        if (d.buildPayloadTree)
            d.buildPayloadTree(typeRoots.emplace_back());
    }
};

} // namespace Miro::Detail
