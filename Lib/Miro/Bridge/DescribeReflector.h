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
    // Record-side mirror of Detail::TypeInfo: owns its strings (the
    // descriptor's string_views point into static type-name buffers,
    // safe to capture here too, but std::string keeps the record
    // self-contained for downstream codegen). `present` reflects
    // whether the corresponding TypeInfo was populated on the
    // descriptor — false for elided sides (void Req/Res).
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
    // Copy TypeInfo → TypeInfoRecord and, if the type is present, run
    // its in-place tree factory into a fresh typeRoots slot. Centralises
    // the (copy strings, conditionally emplace tree) idiom so each
    // descriptor side is one call rather than a six-line block.
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
