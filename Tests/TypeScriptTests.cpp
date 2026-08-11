#include "TestHelpers.h"
#include "TestTypes.h"

#include <NanoTest/NanoTest.h>

using namespace nano;
using namespace Miro;

auto tsImportsZod = test("TypeScript: emits zod import") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "import { z } from \"zod\""));
};

auto tsDeclaresNamedTypes = test("TypeScript: declares named struct types") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "export const Address = z.object({"));
    check(contains(out, "export const User = z.object({"));
    check(contains(out, "export type Address = z.infer<typeof Address>"));
    check(contains(out, "export type User = z.infer<typeof User>"));
};

auto tsDependencyOrder = test("TypeScript: dependencies declared first") = []
{
    auto out = TypeScript::toZod<User>();
    check(comesBefore(out, "export const Address", "export const User"));
};

auto tsPrimitiveSpellings = test("TypeScript: primitive Zod spellings") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "name: z.string()"));
    check(contains(out, "age: z.number().int()"));
    check(contains(out, "active: z.boolean()"));
};

auto tsInt64Zod = test("TypeScript: 64-bit integer fields use z.number().int()") = []
{
    auto out = TypeScript::toZod<ClassWithInt64>();
    check(contains(out, "epochMs: z.number().int()"));
};

auto tsInt64Type = test("TypeScript types: 64-bit integer fields are number") = []
{
    auto out = TypeScript::toTypes<ClassWithInt64>();
    check(contains(out, "epochMs: number;"));
};

auto tsArrayField = test("TypeScript: vector field becomes z.array") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "tags: z.array(z.string())"));
};

auto tsMapField = test("TypeScript: map field becomes z.record") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "counters: z.record(z.string(), z.number().int())"));
};

auto tsOptionalPrimitive =
    test("TypeScript: optional primitive uses .nullish()") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "note: z.string().nullish()"));
};

auto tsOptionalNamed = test("TypeScript: optional named struct uses .nullish()") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "shipping: Address.nullish()"));
};

auto tsNamedReference = test("TypeScript: nested named struct emitted by name") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "address: Address,"));
};

auto tsTopLevelVector =
    test("TypeScript: top-level vector emits default export") = []
{
    auto out = TypeScript::toZod<std::vector<int>>();
    check(contains(out, "export default z.array(z.number().int())"));
};

auto tsTypesInterfaces = test("TypeScript types: emits interfaces") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "export interface Address {"));
    check(contains(out, "export interface User {"));
};

auto tsTypesDependencyOrder = test("TypeScript types: dep order preserved") = []
{
    auto out = TypeScript::toTypes<User>();
    check(comesBefore(out, "export interface Address", "export interface User"));
};

auto tsTypesPrimitiveSpellings = test("TypeScript types: primitive spellings") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "name: string;"));
    check(contains(out, "age: number;"));
    check(contains(out, "active: boolean;"));
};

auto tsTypesArrayField = test("TypeScript types: vector becomes T[]") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "tags: string[];"));
};

auto tsTypesMapField = test("TypeScript types: map becomes Record<string, V>") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "counters: Record<string, number>;"));
};

auto tsTypesOptionalPrimitive =
    test("TypeScript types: optional uses field?: T | null") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "note?: string | null;"));
};

auto tsTypesOptionalNamed =
    test("TypeScript types: optional named struct uses field?: T | null") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "shipping?: Address | null;"));
};

auto tsTypesNamedReference =
    test("TypeScript types: nested named struct emitted by name") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "address: Address;"));
};

auto tsTypesOptionalPrimitiveIsNullable = test(
    "TypeScript types: optional primitive admits the null it serializes to") = []
{
    check(contains(toJSONString(User {}), "\"note\":null"));
    check(contains(TypeScript::toTypes<User>(), "note?: string | null;"));
};

auto tsTypesOptionalNamedIsNullable = test(
    "TypeScript types: optional named struct admits the null it serializes to") = []
{
    check(contains(toJSONString(User {}), "\"shipping\":null"));
    check(contains(TypeScript::toTypes<User>(), "shipping?: Address | null;"));
};

auto tsTypesOptionalEnumIsNullable =
    test("TypeScript types: optional enum admits the null it serializes to") = []
{
    check(contains(toJSONString(User {}), "\"accent\":null"));
    check(contains(TypeScript::toTypes<User>(), "accent?: Color | null;"));
};

auto tsTypesOptionalArrayElementIsNullable =
    test("TypeScript types: optional array element renders (T | null)") = []
{
    auto value = ClassWithOptionalInArray {};
    value.slots.push_back(std::nullopt);
    check(contains(toJSONString(value), "\"slots\":[null]"));
    check(contains(TypeScript::toTypes<ClassWithOptionalInArray>(),
                   "slots: (number | null)[];"));
};

// Non-field optionals must be `.nullable()` (null only), never `.nullish()`,
// which would also admit undefined and drift from the emitted TS type.
auto tsZodTypesNoDriftOnOptionalArrayElement =
    test("TypeScript: zod and types agree on optional array element (no undefined "
         "drift)") = []
{
    auto value = ClassWithOptionalInArray {};
    value.slots.push_back(std::nullopt);

    check(contains(toJSONString(value), "\"slots\":[null]"));
    check(contains(TypeScript::toTypes<ClassWithOptionalInArray>(),
                   "slots: (number | null)[];"));

    auto zod = TypeScript::toZod<ClassWithOptionalInArray>();
    check(contains(zod, "z.array(z.number().int().nullable())"));
    check(!contains(zod, "z.array(z.number().int().nullish())"));
};

auto tsZodTypesNoDriftOnOptionalNamedArrayElement =
    test("TypeScript: zod and types agree on optional named array element") = []
{
    auto value = ClassWithOptionalNamedInArray {};
    value.slots.push_back(std::nullopt);

    check(contains(toJSONString(value), "\"slots\":[null]"));
    check(contains(TypeScript::toTypes<ClassWithOptionalNamedInArray>(),
                   "slots: (Inner | null)[];"));

    auto zod = TypeScript::toZod<ClassWithOptionalNamedInArray>();
    check(contains(zod, "z.array(Inner.nullable())"));
    check(!contains(zod, "z.array(Inner.nullish())"));
};

auto tsZodTypesNoDriftOnOptionalEnumArrayElement =
    test("TypeScript: zod and types agree on optional enum array element") = []
{
    auto value = ClassWithOptionalEnumInArray {};
    value.slots.push_back(std::nullopt);

    check(contains(toJSONString(value), "\"slots\":[null]"));
    check(contains(TypeScript::toTypes<ClassWithOptionalEnumInArray>(),
                   "slots: (Color | null)[];"));

    auto zod = TypeScript::toZod<ClassWithOptionalEnumInArray>();
    check(contains(zod, "z.array(Color.nullable())"));
    check(!contains(zod, "z.array(Color.nullish())"));
};

auto tsZodTypesNoDriftOnOptionalMapValue =
    test("TypeScript: zod and types agree on optional map value") = []
{
    auto value = ClassWithOptionalMapValue {};
    value.slots.emplace("a", std::nullopt);

    check(contains(toJSONString(value), "\"a\":null"));
    check(contains(TypeScript::toTypes<ClassWithOptionalMapValue>(),
                   "slots: Record<string, (number | null)>;"));

    auto zod = TypeScript::toZod<ClassWithOptionalMapValue>();
    check(contains(zod, "z.record(z.string(), z.number().int().nullable())"));
    check(!contains(zod, "z.record(z.string(), z.number().int().nullish())"));
};

auto tsZodOptionalPrimitiveIsNullable =
    test("TypeScript: optional primitive zod admits null (nullish)") = []
{
    check(contains(toJSONString(User {}), "\"note\":null"));
    check(contains(TypeScript::toZod<User>(), "note: z.string().nullish()"));
};

auto tsZodOptionalNamedIsNullable =
    test("TypeScript: optional named struct zod admits null (nullish)") = []
{
    check(contains(toJSONString(User {}), "\"shipping\":null"));
    check(contains(TypeScript::toZod<User>(), "shipping: Address.nullish()"));
};

auto tsZodOptionalEnumIsNullable =
    test("TypeScript: optional enum zod admits null (nullish)") = []
{
    check(contains(toJSONString(User {}), "\"accent\":null"));
    check(contains(TypeScript::toZod<User>(), "accent: Color.nullish()"));
};

auto tsTypesNoZodImport = test("TypeScript types: does not import zod") = []
{
    auto out = TypeScript::toTypes<User>();
    check(!contains(out, "from \"zod\""));
};

auto tsTypesTopLevelVector =
    test("TypeScript types: top-level vector becomes Root alias") = []
{
    auto out = TypeScript::toTypes<std::vector<int>>();
    check(contains(out, "export type Root = number[];"));
};

auto tsEnumDeclaresZodEnum =
    test("TypeScript: enum becomes top-level z.enum declaration") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out,
                   "export const Color = z.enum([\"Red\", \"Green\", \"Blue\"]);"));
    check(contains(out, "export type Color = z.infer<typeof Color>;"));
};

auto tsEnumWithExplicitBase =
    test("TypeScript: enum with explicit underlying type emits all enumerators") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(
        out, "export const Priority = z.enum([\"Low\", \"Medium\", \"High\"]);"));
};

auto tsEnumFieldRefersByName =
    test("TypeScript: enum field references the named enum") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "color: Color,"));
    check(contains(out, "priority: Priority,"));
};

auto tsEnumOptional = test("TypeScript: optional enum uses .nullish()") = []
{
    auto out = TypeScript::toZod<User>();
    check(contains(out, "accent: Color.nullish()"));
};

auto tsEnumDependencyOrder =
    test("TypeScript: enum declared before struct that uses it") = []
{
    auto out = TypeScript::toZod<User>();
    check(comesBefore(out, "export const Color", "export const User"));
    check(comesBefore(out, "export const Priority", "export const User"));
};

auto tsTypesEnumIsLiteralUnion =
    test("TypeScript types: enum becomes string-literal union alias") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "export type Color = \"Red\" | \"Green\" | \"Blue\";"));
    check(contains(out, "export type Priority = \"Low\" | \"Medium\" | \"High\";"));
};

auto tsTypesEnumFieldRefersByName =
    test("TypeScript types: enum field references the named enum") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "color: Color;"));
    check(contains(out, "priority: Priority;"));
};

auto tsTypesEnumOptional =
    test("TypeScript types: optional enum uses field?: T | null") = []
{
    auto out = TypeScript::toTypes<User>();
    check(contains(out, "accent?: Color | null;"));
};

auto tsTypesEnumDependencyOrder =
    test("TypeScript types: enum declared before struct that uses it") = []
{
    auto out = TypeScript::toTypes<User>();
    check(comesBefore(out, "export type Color", "export interface User"));
};

namespace
{

struct CustomKeys
{
    int xCoord = 0;
    std::string fullName;
    bool active = false;

    MIRO_REFLECT_MEMBERS(xCoord, "X Coord", fullName, "Full Name", active, "active")
};

} // namespace

auto tsZodQuotesNonIdentifierKeys =
    test("TypeScript: zod quotes keys that aren't valid JS identifiers") = []
{
    auto out = TypeScript::toZod<CustomKeys>();
    check(contains(out, "\"X Coord\": z.number().int(),"));
    check(contains(out, "\"Full Name\": z.string(),"));
};

auto tsZodLeavesIdentifierKeysBare =
    test("TypeScript: zod leaves valid identifier keys unquoted") = []
{
    auto out = TypeScript::toZod<CustomKeys>();
    check(contains(out, "active: z.boolean(),"));
    check(!contains(out, "\"active\":"));
};

auto tsTypesQuotesNonIdentifierKeys = test(
    "TypeScript types: interface quotes keys that aren't valid identifiers") = []
{
    auto out = TypeScript::toTypes<CustomKeys>();
    check(contains(out, "\"X Coord\": number;"));
    check(contains(out, "\"Full Name\": string;"));
};

auto tsTypesLeavesIdentifierKeysBare =
    test("TypeScript types: interface leaves valid identifier keys unquoted") = []
{
    auto out = TypeScript::toTypes<CustomKeys>();
    check(contains(out, "active: boolean;"));
    check(!contains(out, "\"active\":"));
};

// These sit at file scope on purpose: an enclosing anonymous namespace would
// prefix the qualified name with "(anonymous namespace)::" and break the
// sanitized-name assertions below.

namespace TsCollAlpha
{
struct Item
{
    int id = 0;

    MIRO_REFLECT(id)
};
} // namespace TsCollAlpha

namespace TsCollBeta
{
struct Item
{
    bool flag = false;

    MIRO_REFLECT(flag)
};
} // namespace TsCollBeta

namespace TsCollSolo
{
struct Solo
{
    int x = 0;

    MIRO_REFLECT(x)
};
} // namespace TsCollSolo

namespace
{

struct CollidingPair
{
    TsCollAlpha::Item alpha;
    TsCollBeta::Item beta;

    MIRO_REFLECT(alpha, beta)
};

struct UsesSolo
{
    TsCollSolo::Solo solo;

    MIRO_REFLECT(solo)
};

} // namespace

auto tsZodDisambiguatesColliding =
    test("TypeScript: same short name from different namespaces "
         "gets sanitized qualified declarations in zod") = []
{
    auto out = TypeScript::toZod<CollidingPair>();

    check(contains(out, "export const TsCollAlpha_Item = z.object({"));
    check(contains(out, "export const TsCollBeta_Item = z.object({"));
    check(contains(out, "id: z.number().int(),"));
    check(contains(out, "flag: z.boolean(),"));

    check(!contains(out, "export const Item = "));
};

auto tsZodReferencesUseDisambiguatedNames =
    test("TypeScript: field references resolve to the disambiguated names") = []
{
    auto out = TypeScript::toZod<CollidingPair>();

    check(contains(out, "alpha: TsCollAlpha_Item,"));
    check(contains(out, "beta: TsCollBeta_Item,"));
};

auto tsTypesDisambiguatesColliding =
    test("TypeScript types: collision produces sanitized qualified interfaces") = []
{
    auto out = TypeScript::toTypes<CollidingPair>();

    check(contains(out, "export interface TsCollAlpha_Item {"));
    check(contains(out, "export interface TsCollBeta_Item {"));
    check(contains(out, "alpha: TsCollAlpha_Item;"));
    check(contains(out, "beta: TsCollBeta_Item;"));
    check(!contains(out, "export interface Item "));
};

auto tsKeepsShortNameWhenNoCollision =
    test("TypeScript: namespaced type keeps its short name when no collision") = []
{
    auto out = TypeScript::toZod<UsesSolo>();

    check(contains(out, "export const Solo = z.object({"));
    check(contains(out, "solo: Solo,"));
    check(!contains(out, "TsCollSolo_Solo"));
};
