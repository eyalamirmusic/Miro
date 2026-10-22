#pragma once

#include "../JSON/Json.h"
#include "../TypeTree/TypeTree.h"

#include <span>
#include <string>

namespace Miro::Schema
{

// The flavour of JSON Schema formatJsonSchema emits. A default-
// constructed Dialect is the library's historical output (standard()):
// "nullable": true on optional slots, optional and omittable fields
// left out of "required", open objects, and tagged unions as a oneOf of
// allOf [tag constraint, arm].
//
// openAiStrict() targets OpenAI function calling / structured outputs
// in strict mode: every object closed, every property required (an
// optional or omittable field becomes nullable instead), null spelled
// as a type union, unions as an anyOf of flattened, closed arms, and
// no vendor "x-" keywords. Maps and raw JSON (Miro::JSON) fields can't
// be expressed there; reaching one throws std::invalid_argument naming
// the field path.
struct Dialect
{
    enum class NullStyle
    {
        // `"nullable": true` next to the value's own schema.
        Keyword,

        // `"type": ["string", "null"]` where the schema is a plain
        // `type`, otherwise `"anyOf": [<schema>, {"type": "null"}]`
        // (e.g. a $ref to a named object, enum or union).
        TypeUnion,
    };

    enum class UnionStyle
    {
        // oneOf of allOf [{tag: const}, arm], shared plain fields as an
        // outer allOf.
        OneOf,

        // anyOf of one object per arm, holding the tag as a single-value
        // enum, the shared plain fields and the arm's own fields. No
        // allOf, so it composes with closedObjects.
        AnyOf,
    };

    static Dialect standard();
    static Dialect openAiStrict();

    // "additionalProperties": false on every object. Map and raw JSON
    // (Any) slots throw std::invalid_argument when this is set.
    bool closedObjects {};

    // Every property is listed in "required"; optional and omittable
    // fields are expressed as nullable instead of absent.
    bool allRequired {};

    // Leave out vendor extension keywords ("x-enumNames").
    bool stripExtensions {};

    NullStyle nullStyle {};
    UnionStyle unionStyle {};
};

} // namespace Miro::Schema

namespace Miro
{

// Renders a TypeNode tree (or several roots) as a JSON Schema document.
// Every named type's body lives under a top-level "$defs" map; every
// reference (including recursive ones) becomes {"$ref": "..."}.
//
// Roots are passed mutably because the disambiguation pass may rewrite
// per-node `typeName` when distinct C++ types share an unqualified
// name. Callers that don't want the trees touched should hand over a
// copy.
JSON formatJsonSchema(TypeTree::TypeNode& root,
                      const Schema::Dialect& dialect = Schema::Dialect::standard());
JSON formatJsonSchema(std::span<TypeTree::TypeNode> roots,
                      const Schema::Dialect& dialect = Schema::Dialect::standard());

template <typename T>
JSON schemaOf(const Schema::Dialect& dialect = Schema::Dialect::standard())
{
    auto tree = TypeTree::buildTree<T>();
    return formatJsonSchema(tree, dialect);
}

template <typename T>
std::string
    schemaOfAsString(int indent = 2,
                     const Schema::Dialect& dialect = Schema::Dialect::standard())
{
    return Json::print(schemaOf<T>(dialect), indent);
}

template <typename T>
std::string schemaOfAsString(const Schema::Dialect& dialect, int indent = 2)
{
    return schemaOfAsString<T>(indent, dialect);
}

} // namespace Miro
