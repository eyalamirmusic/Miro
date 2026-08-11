#pragma once

#include "../TypeTree/TypeTree.h"
#include "ApiReflector.h"

#include <ea_data_structures/Structures/Vector.h>

#include <string>

namespace Miro::Detail
{

class DescribeReflector : public ApiReflector
{
public:
    struct TypeInfoRecord
    {
        std::string name;
        std::string qualifiedName;
        bool present = false;

        explicit operator bool() const { return present; }
    };

    struct CommandRecord
    {
        std::string name;
        TypeInfoRecord req;
        TypeInfoRecord res;
    };

    struct EventRecord
    {
        std::string name;
        TypeInfoRecord payload;
        std::function<JSON()> defaultPayloadJson;

        bool isKeyed = false;
        std::string collectionField;
        std::string keyField;
    };

    Vector<CommandRecord> commands;
    Vector<EventRecord> events;

    // Duplicates are deliberately kept — TypeTree::prepareRoots dedupes
    // by qualifiedName at format time.
    Vector<TypeTree::TypeNode> typeRoots;

protected:
    void commandImpl(const CommandDescriptor& d) override
    {
        auto record = CommandRecord {};
        record.name = std::string {d.name};
        recordTypeInfo(d.req, record.req);
        recordTypeInfo(d.res, record.res);
        commands.add(std::move(record));
    }

    void eventImpl(const EventDescriptor& d) override
    {
        auto record = EventRecord {};
        record.name = std::string {d.name};
        recordTypeInfo(d.payload, record.payload);
        record.defaultPayloadJson = d.defaultPayloadJson;
        record.isKeyed = d.isKeyed;
        record.collectionField = std::string {d.collectionField};
        record.keyField = std::string {d.keyField};
        events.add(std::move(record));
    }

private:
    void recordTypeInfo(const TypeInfo& src, TypeInfoRecord& dst)
    {
        if (!src)
            return;
        dst.name = std::string {src.name};
        dst.qualifiedName = std::string {src.qualifiedName};
        dst.present = true;
        src.buildTree(typeRoots.emplace_back());
    }
};

} // namespace Miro::Detail
