#include "TypeScript.h"

#include "../CommandExport/ResolvedTypes.h"
#include "../Detail/StringUtilities.h"

#include <ResEmbed/ResEmbed.h>

#include <sstream>
#include <string>
#include <string_view>

namespace Miro::TypeScript
{

using TypeTree::TypeNode;

namespace
{

// Returns `name` ready to drop into a JS object literal or TS interface
// as a property key. Bare identifier when possible; otherwise a JSON-
// quoted string with `\` and `"` escaped.
std::string formatPropertyKey(std::string_view name)
{
    if (Detail::isJsIdentifier(name))
        return std::string {name};

    auto out = std::string {"\""};
    for (auto c: name)
    {
        if (c == '\\' || c == '"')
            out += '\\';
        out += c;
    }
    out += '"';
    return out;
}

std::string_view zodPrimitive(TypeTree::PrimitiveKind kind)
{
    switch (kind)
    {
        case TypeTree::PrimitiveKind::Boolean:
            return "z.boolean()";
        case TypeTree::PrimitiveKind::String:
            return "z.string()";
        case TypeTree::PrimitiveKind::Number:
            return "z.number()";
        case TypeTree::PrimitiveKind::Integer:
        case TypeTree::PrimitiveKind::Int64:
            return "z.number().int()";
    }
    return "z.unknown()";
}

std::string_view tsPrimitive(TypeTree::PrimitiveKind kind)
{
    switch (kind)
    {
        case TypeTree::PrimitiveKind::Boolean:
            return "boolean";
        case TypeTree::PrimitiveKind::String:
            return "string";
        case TypeTree::PrimitiveKind::Number:
        case TypeTree::PrimitiveKind::Integer:
        case TypeTree::PrimitiveKind::Int64:
            return "number";
    }
    return "unknown";
}

// ---------- Zod renderer ----------

std::string renderZod(const TypeNode& node, bool fieldContext);

std::string renderZodObjectInline(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "z.object({\n";

    for (auto& field: node.fields)
        out << "    " << formatPropertyKey(field.name) << ": "
            << renderZod(*field.type, /*fieldContext=*/true) << ",\n";

    out << "})";
    return out.str();
}

std::string renderZodEnumInline(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "z.enum([";

    auto first = true;
    for (auto& value: node.enumValues)
    {
        if (!first)
            out << ", ";
        first = false;
        out << "\"" << value << "\"";
    }

    out << "])";
    return out.str();
}

std::string renderZod(const TypeNode& node, bool fieldContext)
{
    auto base = std::string {};

    switch (node.shape)
    {
        case TypeNode::Shape::Primitive:
            base = std::string {zodPrimitive(node.primitive)};
            break;
        case TypeNode::Shape::Object:
            base =
                node.typeName.empty() ? renderZodObjectInline(node) : node.typeName;
            break;
        case TypeNode::Shape::Array:
            base = "z.array(" + renderZod(*node.inner, /*fieldContext=*/false) + ")";
            break;
        case TypeNode::Shape::Map:
            base = "z.record(z.string(), "
                   + renderZod(*node.inner, /*fieldContext=*/false) + ")";
            break;
        case TypeNode::Shape::Enum:
            base = node.typeName.empty() ? renderZodEnumInline(node) : node.typeName;
            break;
    }

    // An empty std::optional serializes to JSON `null`, never `undefined`
    // (ReflectContainers `writeNull`), so the schema must admit `null`. In
    // field context `.nullish()` (= null | undefined) also makes the object
    // key optional, mirroring the plain renderer's `field?: T | null`. Outside
    // a field (array element, map value) there is no absent case — a JSON
    // array/object value is `null`, never `undefined` — so `.nullable()`
    // (null only) keeps the inferred type identical to the types module's
    // `(T | null)`. Using `.nullish()` here would add a spurious `| undefined`
    // and drift from the plain types.
    if (node.optional)
        base += fieldContext ? ".nullish()" : ".nullable()";

    return base;
}

std::string declareZodNamed(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "export const " << node.typeName << " = " << renderZodObjectInline(node)
        << ";\n";
    out << "export type " << node.typeName << " = z.infer<typeof " << node.typeName
        << ">;\n";
    return out.str();
}

std::string declareZodEnum(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "export const " << node.typeName << " = " << renderZodEnumInline(node)
        << ";\n";
    out << "export type " << node.typeName << " = z.infer<typeof " << node.typeName
        << ">;\n";
    return out.str();
}

// ---------- Plain TypeScript renderer ----------

// Renders a node as a TypeScript type expression. Field-level absence is
// carried by the `field?:` separator; the disengaged-optional value (which
// serializes to JSON `null`) is carried by a `| null` union added here, in
// both field and non-field contexts.
std::string renderType(const TypeNode& node, bool fieldContext);

std::string renderTypeObjectInline(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "{\n";

    for (auto& field: node.fields)
    {
        auto separator = field.type->optional ? "?: " : ": ";
        out << "    " << formatPropertyKey(field.name) << separator
            << renderType(*field.type, /*fieldContext=*/true) << ";\n";
    }

    out << "}";
    return out.str();
}

std::string renderTypeEnumInline(const TypeNode& node)
{
    auto out = std::string {};
    auto first = true;

    for (auto& value: node.enumValues)
    {
        if (!first)
            out += " | ";
        first = false;
        out += "\"";
        out += value;
        out += "\"";
    }

    return out;
}

std::string renderType(const TypeNode& node, bool fieldContext)
{
    auto base = std::string {};

    switch (node.shape)
    {
        case TypeNode::Shape::Primitive:
            base = std::string {tsPrimitive(node.primitive)};
            break;
        case TypeNode::Shape::Object:
            base =
                node.typeName.empty() ? renderTypeObjectInline(node) : node.typeName;
            break;
        case TypeNode::Shape::Array:
            base = renderType(*node.inner, /*fieldContext=*/false) + "[]";
            break;
        case TypeNode::Shape::Map:
            base = "Record<string, " + renderType(*node.inner, false) + ">";
            break;
        case TypeNode::Shape::Enum:
            base =
                node.typeName.empty() ? renderTypeEnumInline(node) : node.typeName;
            break;
    }

    // An empty std::optional serializes to JSON `null` (ReflectContainers
    // `writeNull`), so the type must admit `null`. In field context the
    // `field?:` separator already conveys absent/undefined; we add `| null`
    // for the disengaged-value case. Outside a field (e.g. the inner of an
    // array) the whole union is wrapped, since there is no `?:` to carry it.
    if (node.optional)
        base = fieldContext ? base + " | null" : "(" + base + " | null)";

    return base;
}

std::string declareTypeNamed(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "export interface " << node.typeName << " "
        << renderTypeObjectInline(node) << "\n";
    return out.str();
}

std::string declareTypeEnum(const TypeNode& node)
{
    auto out = std::ostringstream {};
    out << "export type " << node.typeName << " = " << renderTypeEnumInline(node)
        << ";\n";
    return out.str();
}

// True if the root node was emitted as its own top-level declaration by
// collectNamed — in that case we skip the default-export / Root-alias
// fallback to avoid a redundant second name for the same shape.
bool rootIsHoisted(const TypeNode& root)
{
    if (root.typeName.empty())
        return false;

    return root.shape == TypeNode::Shape::Object
           || root.shape == TypeNode::Shape::Enum;
}

} // namespace

std::string formatZodModule(std::span<TypeNode> roots)
{
    auto ordered = TypeTree::prepareRoots(roots);

    auto out = std::ostringstream {};
    out << "import { z } from \"zod\";\n\n";

    for (auto* node: ordered)
    {
        if (node->shape == TypeNode::Shape::Enum)
            out << declareZodEnum(*node) << "\n";
        else
            out << declareZodNamed(*node) << "\n";
    }

    return out.str();
}

std::string formatTypesModule(std::span<TypeNode> roots)
{
    auto ordered = TypeTree::prepareRoots(roots);

    auto out = std::ostringstream {};

    for (auto* node: ordered)
    {
        if (node->shape == TypeNode::Shape::Enum)
            out << declareTypeEnum(*node) << "\n";
        else
            out << declareTypeNamed(*node) << "\n";
    }

    return out.str();
}

std::string formatZodModule(TypeNode& root)
{
    auto out = formatZodModule(std::span<TypeNode> {&root, 1});

    // The bundled overload skips default exports (one-per-module rule);
    // the single-root path adds one for anonymous roots like top-level
    // vectors so the module isn't pointless.
    if (!rootIsHoisted(root))
        out += "export default " + renderZod(root, /*fieldContext=*/false) + ";\n";

    return out;
}

std::string formatTypesModule(TypeNode& root)
{
    auto out = formatTypesModule(std::span<TypeNode> {&root, 1});

    if (!rootIsHoisted(root))
        out +=
            "export type Root = " + renderType(root, /*fieldContext=*/false) + ";\n";

    return out;
}

std::string formatBridgeRuntime()
{
    return ResEmbed::get("BridgeRuntime.ts", "MiroResources").toString();
}

std::string formatEventsModule(std::span<TypeNode> typeRoots,
                               std::span<const TypeExport::EventInfo> events,
                               std::string_view baseName)
{
    auto resolved = CommandExport::resolveTypes(typeRoots);

    auto out = std::ostringstream {};
    out << "import type * as T from './" << baseName << "';\n\n";

    out << "export interface Events\n{\n";
    for (auto& ev: events)
    {
        auto payload =
            resolved.nameFor(ev.payloadQualifiedName, ev.payloadTypeName);
        out << "    '" << ev.name << "': T." << payload << ";\n";
    }
    out << "}\n\n";

    out << "export type EventName = keyof Events;\n\n";

    out << "export interface EventBus\n"
           "{\n"
           "    subscribe<K extends EventName>(\n"
           "        name: K,\n"
           "        handler: (payload: Events[K]) => void,\n"
           "    ): () => void;\n"
           "}\n";

    return out.str();
}

} // namespace Miro::TypeScript
