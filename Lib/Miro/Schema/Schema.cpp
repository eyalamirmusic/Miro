#include "Schema.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace Miro
{

namespace
{

// Replaces every non-identifier run in `raw` with a single `_` and
// strips leading underscores/digits so the result is a legal JSON
// pointer fragment. Mirrors TypeScript's sanitizeIdentifier; we strip
// "(anonymous namespace)::" first so test types nested in anonymous
// namespaces don't get hideous keys.
std::string sanitizeForDefs(std::string_view raw)
{
    constexpr auto anonPrefix = std::string_view {"(anonymous namespace)::"};
    auto trimmed = raw;
    while (trimmed.starts_with(anonPrefix))
        trimmed.remove_prefix(anonPrefix.size());

    auto isIdChar = [](char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
               || (c >= '0' && c <= '9') || c == '_';
    };

    auto out = std::string {};
    out.reserve(trimmed.size());

    for (auto c: trimmed)
    {
        if (isIdChar(c))
            out += c;
        else if (!out.empty() && out.back() != '_')
            out += '_';
    }

    while (!out.empty() && out.back() == '_')
        out.pop_back();
    while (!out.empty()
           && (out.front() == '_' || (out.front() >= '0' && out.front() <= '9')))
        out.erase(0, 1);

    return out;
}

} // namespace

SchemaReflector::SchemaReflector(Json::Value& nodeToUse,
                                 Options optsToUse,
                                 Detail::SchemaContext* contextToUse)
    : Reflector(optsToUse)
    , node(&nodeToUse)
    , context(contextToUse)
{
    commitShape();

    if (opts.shape != Shape::Primitive)
        applyNullable();
}

SchemaReflector::~SchemaReflector() = default;

ValueKind SchemaReflector::kind() const
{
    return ValueKind::Absent;
}

void SchemaReflector::writeNull() {}

Json::Value SchemaReflector::primitiveSchema(std::string_view typeName)
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {typeName}};
    return value;
}

Json::Value SchemaReflector::objectSchema()
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"object"}};
    value.asObject()["properties"] = Json::Value {Json::Object {}};
    return value;
}

Json::Value SchemaReflector::arraySchema()
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"array"}};
    value.asObject()["items"] = Json::Value {Json::Object {}};
    return value;
}

Json::Value SchemaReflector::mapSchema()
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"object"}};
    value.asObject()["additionalProperties"] = Json::Value {Json::Object {}};
    return value;
}

Json::Value SchemaReflector::enumSchema(const std::vector<std::string_view>& names)
{
    auto value = Json::Value {Json::Object {}};
    value.asObject()["type"] = Json::Value {std::string {"string"}};

    if (!names.empty())
    {
        auto arr = Json::Array {};
        arr.reserve(names.size());

        for (auto& name: names)
            arr.emplace_back(std::string {name});

        value.asObject()["enum"] = Json::Value {std::move(arr)};
    }

    return value;
}

void SchemaReflector::commitShape()
{
    switch (opts.shape)
    {
        case Shape::Primitive:
            // visit() will write the {type:...} object once it knows
            // the concrete primitive type.
            break;
        case Shape::Object:
            *node = objectSchema();
            break;
        case Shape::Array:
            *node = arraySchema();
            break;
        case Shape::Map:
            *node = mapSchema();
            break;
    }
}

void SchemaReflector::applyNullable()
{
    if (!opts.nullable)
        return;

    if (!node->isObject())
        *node = Json::Value {Json::Object {}};

    node->asObject()["nullable"] = Json::Value {true};
}

void SchemaReflector::visit(PrimitiveRef ref)
{
    std::visit(
        [this](auto* ptr)
        {
            using T = std::remove_pointer_t<decltype(ptr)>;
            auto typeName = std::string_view {};
            if constexpr (std::same_as<T, bool>)
                typeName = "boolean";
            else if constexpr (std::same_as<T, std::string>)
                typeName = "string";
            else if constexpr (std::same_as<T, double>)
                typeName = "number";
            else
                typeName = "integer";

            *node = primitiveSchema(typeName);
            applyNullable();
        },
        ref.data);
}

void SchemaReflector::visitEnum(TypeId, const std::vector<std::string_view>& names)
{
    *node = enumSchema(names);
    applyNullable();
}

void SchemaReflector::replaceWithRef(const std::string& defsName)
{
    *node = Json::Value {Json::Object {}};
    node->asObject()["$ref"] = Json::Value {std::string {"#/$defs/"} + defsName};

    // The reference inherits this slot's nullability — the named type's
    // own schema lives in $defs and isn't itself nullable, only this
    // particular use of it.
    if (opts.nullable)
        node->asObject()["nullable"] = Json::Value {true};
}

bool SchemaReflector::beginNamedType(TypeId id)
{
    if (context == nullptr)
        return true;

    auto key = sanitizeForDefs(id.qualifiedName);
    auto& defsObj = context->defs.asObject();

    // Already known (or in flight) — emit just a $ref and skip the body.
    // Inserting into `known` before recursing means the in-flight body
    // walking deeper will see the type as known and emit refs for back
    // edges, which is what breaks recursion.
    if (context->known.contains(key))
    {
        replaceWithRef(key);
        return false;
    }

    context->known.insert(key);

    // Stamp a $ref into the original slot, then redirect this reflector
    // to write the body into $defs[key]. We re-commit the shape there
    // because `commitShape` ran on the original slot during construction
    // and we just overwrote it with the $ref.
    replaceWithRef(key);
    defsObj[key] = Json::Value {Json::Object {}};
    node = &defsObj[key];
    commitShape();

    return true;
}

Reflector& SchemaReflector::spawnChild(Json::Value& targetNode, Options childOpts)
{
    // Destroy previous child first; see JsonReflector::spawnChild.
    currentChild.reset();
    currentChild = std::make_unique<SchemaReflector>(targetNode, childOpts, context);
    return *currentChild;
}

Reflector& SchemaReflector::atKey(std::string_view key, Options childOpts)
{
    if (opts.shape == Shape::Map)
        return spawnChild(node->asObject()["additionalProperties"], childOpts);

    auto& props = node->asObject()["properties"];
    if (!props.isObject())
        props = Json::Value {Json::Object {}};

    if (!childOpts.nullable)
        appendRequired(key);

    return spawnChild(props.asObject()[std::string {key}], childOpts);
}

Reflector& SchemaReflector::atIndex(std::size_t, Options childOpts)
{
    return spawnChild(node->asObject()["items"], childOpts);
}

void SchemaReflector::setArrayBounds(std::size_t min, std::size_t max)
{
    if (!node->isObject())
        return;

    auto& obj = node->asObject();
    obj["minItems"] = Json::Value {static_cast<double>(min)};
    obj["maxItems"] = Json::Value {static_cast<double>(max)};
}

void SchemaReflector::appendRequired(std::string_view key)
{
    auto& obj = node->asObject();
    auto it = obj.find("required");

    if (it == obj.end())
    {
        auto arr = Json::Array {};
        arr.emplace_back(std::string {key});
        obj["required"] = Json::Value {std::move(arr)};
        return;
    }

    std::get<Json::Array>(it->second.data).emplace_back(std::string {key});
}

} // namespace Miro
