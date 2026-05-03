// Tests for the Zod / TypeScript exporter. Stage-1 strategy: make
// substring-level assertions on the generated text. Real TS-compiler
// validation is a future addition.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <string>

using namespace nano;

namespace
{

struct Address
{
    std::string street;
    std::string zip;

    MIRO_REFLECT(street, zip)
};

enum class Color
{
    Red,
    Green,
    Blue
};

enum class Priority : int
{
    Low = -1,
    Medium = 0,
    High = 1
};

struct User
{
    std::string name;
    int age = 0;
    bool active = true;
    Address address;
    std::vector<std::string> tags;
    std::map<std::string, int> counters;
    std::optional<std::string> note;
    std::optional<Address> shipping;
    Color color = Color::Red;
    Priority priority = Priority::Medium;
    std::optional<Color> accent;

    MIRO_REFLECT(name,
                 age,
                 active,
                 address,
                 tags,
                 counters,
                 note,
                 shipping,
                 color,
                 priority,
                 accent)
};

bool contains(const std::string& haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string::npos;
}

bool comesBefore(const std::string& s,
                 std::string_view earlier,
                 std::string_view later)
{
    auto e = s.find(earlier);
    auto l = s.find(later);
    return e != std::string::npos && l != std::string::npos && e < l;
}

} // namespace

auto tsImportsZod = test("TypeScript: emits zod import") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "import { z } from \"zod\""));
};

auto tsDeclaresNamedTypes = test("TypeScript: declares named struct types") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "export const Address = z.object({"));
    check(contains(out, "export const User = z.object({"));
    check(contains(out, "export type Address = z.infer<typeof Address>"));
    check(contains(out, "export type User = z.infer<typeof User>"));
};

auto tsDependencyOrder = test("TypeScript: dependencies declared first") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(comesBefore(out, "export const Address", "export const User"));
};

auto tsPrimitiveSpellings = test("TypeScript: primitive Zod spellings") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "name: z.string()"));
    check(contains(out, "age: z.number().int()"));
    check(contains(out, "active: z.boolean()"));
};

auto tsArrayField = test("TypeScript: vector field becomes z.array") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "tags: z.array(z.string())"));
};

auto tsMapField = test("TypeScript: map field becomes z.record") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "counters: z.record(z.string(), z.number().int())"));
};

auto tsOptionalPrimitive =
    test("TypeScript: optional primitive uses .optional()") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "note: z.string().optional()"));
};

auto tsOptionalNamed =
    test("TypeScript: optional named struct uses .optional()") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "shipping: Address.optional()"));
};

auto tsNamedReference = test("TypeScript: nested named struct emitted by name") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "address: Address,"));
};

auto tsTopLevelVector =
    test("TypeScript: top-level vector emits default export") = []
{
    auto out = Miro::TypeScript::toZod<std::vector<int>>();
    check(contains(out, "export default z.array(z.number().int())"));
};

// ---------- Plain-TS exporter tests ----------

auto tsTypesInterfaces = test("TypeScript types: emits interfaces") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "export interface Address {"));
    check(contains(out, "export interface User {"));
};

auto tsTypesDependencyOrder = test("TypeScript types: dep order preserved") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(comesBefore(out, "export interface Address", "export interface User"));
};

auto tsTypesPrimitiveSpellings = test("TypeScript types: primitive spellings") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "name: string;"));
    check(contains(out, "age: number;"));
    check(contains(out, "active: boolean;"));
};

auto tsTypesArrayField = test("TypeScript types: vector becomes T[]") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "tags: string[];"));
};

auto tsTypesMapField = test("TypeScript types: map becomes Record<string, V>") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "counters: Record<string, number>;"));
};

auto tsTypesOptionalPrimitive = test("TypeScript types: optional uses field?:") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "note?: string;"));
};

auto tsTypesOptionalNamed =
    test("TypeScript types: optional named struct uses field?:") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "shipping?: Address;"));
};

auto tsTypesNamedReference =
    test("TypeScript types: nested named struct emitted by name") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "address: Address;"));
};

auto tsTypesNoZodImport = test("TypeScript types: does not import zod") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(!contains(out, "from \"zod\""));
};

auto tsTypesTopLevelVector =
    test("TypeScript types: top-level vector becomes Root alias") = []
{
    auto out = Miro::TypeScript::toTypes<std::vector<int>>();
    check(contains(out, "export type Root = number[];"));
};

// ---------- Enum exporter tests ----------

auto tsEnumDeclaresZodEnum =
    test("TypeScript: enum becomes top-level z.enum declaration") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out,
                   "export const Color = z.enum([\"Red\", \"Green\", \"Blue\"]);"));
    check(contains(out, "export type Color = z.infer<typeof Color>;"));
};

auto tsEnumWithExplicitBase =
    test("TypeScript: enum with explicit underlying type emits all enumerators") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(
        out, "export const Priority = z.enum([\"Low\", \"Medium\", \"High\"]);"));
};

auto tsEnumFieldRefersByName =
    test("TypeScript: enum field references the named enum") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "color: Color,"));
    check(contains(out, "priority: Priority,"));
};

auto tsEnumOptional = test("TypeScript: optional enum uses .optional()") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(contains(out, "accent: Color.optional()"));
};

auto tsEnumDependencyOrder =
    test("TypeScript: enum declared before struct that uses it") = []
{
    auto out = Miro::TypeScript::toZod<User>();
    check(comesBefore(out, "export const Color", "export const User"));
    check(comesBefore(out, "export const Priority", "export const User"));
};

auto tsTypesEnumIsLiteralUnion =
    test("TypeScript types: enum becomes string-literal union alias") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "export type Color = \"Red\" | \"Green\" | \"Blue\";"));
    check(contains(out, "export type Priority = \"Low\" | \"Medium\" | \"High\";"));
};

auto tsTypesEnumFieldRefersByName =
    test("TypeScript types: enum field references the named enum") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "color: Color;"));
    check(contains(out, "priority: Priority;"));
};

auto tsTypesEnumOptional = test("TypeScript types: optional enum uses field?:") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(contains(out, "accent?: Color;"));
};

auto tsTypesEnumDependencyOrder =
    test("TypeScript types: enum declared before struct that uses it") = []
{
    auto out = Miro::TypeScript::toTypes<User>();
    check(comesBefore(out, "export type Color", "export interface User"));
};

// ---------- Custom-key (MIRO_REFLECT_MEMBERS) tests ----------

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
    auto out = Miro::TypeScript::toZod<CustomKeys>();
    check(contains(out, "\"X Coord\": z.number().int(),"));
    check(contains(out, "\"Full Name\": z.string(),"));
};

auto tsZodLeavesIdentifierKeysBare =
    test("TypeScript: zod leaves valid identifier keys unquoted") = []
{
    auto out = Miro::TypeScript::toZod<CustomKeys>();
    check(contains(out, "active: z.boolean(),"));
    check(!contains(out, "\"active\":"));
};

auto tsTypesQuotesNonIdentifierKeys = test(
    "TypeScript types: interface quotes keys that aren't valid identifiers") = []
{
    auto out = Miro::TypeScript::toTypes<CustomKeys>();
    check(contains(out, "\"X Coord\": number;"));
    check(contains(out, "\"Full Name\": string;"));
};

auto tsTypesLeavesIdentifierKeysBare =
    test("TypeScript types: interface leaves valid identifier keys unquoted") = []
{
    auto out = Miro::TypeScript::toTypes<CustomKeys>();
    check(contains(out, "active: boolean;"));
    check(!contains(out, "\"active\":"));
};

// ---------- Namespace-disambiguation tests ----------
//
// These namespaces live at file scope (rather than nested inside an
// anonymous namespace like the other test types) because the
// disambiguation pass sanitizes the raw qualified name returned by
// __PRETTY_FUNCTION__. Wrapping them in an anonymous namespace would
// pollute the qualified name with "(anonymous namespace)::" and break
// the assertions below. The namespace names are unique-prefixed so
// they can safely sit at file scope without colliding with anything
// else in the test executable.

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
    auto out = Miro::TypeScript::toZod<CollidingPair>();

    check(contains(out, "export const TsCollAlpha_Item = z.object({"));
    check(contains(out, "export const TsCollBeta_Item = z.object({"));
    check(contains(out, "id: z.number().int(),"));
    check(contains(out, "flag: z.boolean(),"));

    // The plain "Item" must not appear as a declaration anymore.
    check(!contains(out, "export const Item = "));
};

auto tsZodReferencesUseDisambiguatedNames =
    test("TypeScript: field references resolve to the disambiguated names") = []
{
    auto out = Miro::TypeScript::toZod<CollidingPair>();

    check(contains(out, "alpha: TsCollAlpha_Item,"));
    check(contains(out, "beta: TsCollBeta_Item,"));
};

auto tsTypesDisambiguatesColliding =
    test("TypeScript types: collision produces sanitized qualified interfaces") = []
{
    auto out = Miro::TypeScript::toTypes<CollidingPair>();

    check(contains(out, "export interface TsCollAlpha_Item {"));
    check(contains(out, "export interface TsCollBeta_Item {"));
    check(contains(out, "alpha: TsCollAlpha_Item;"));
    check(contains(out, "beta: TsCollBeta_Item;"));
    check(!contains(out, "export interface Item "));
};

auto tsKeepsShortNameWhenNoCollision =
    test("TypeScript: namespaced type keeps its short name when no collision") = []
{
    auto out = Miro::TypeScript::toZod<UsesSolo>();

    check(contains(out, "export const Solo = z.object({"));
    check(contains(out, "solo: Solo,"));
    check(!contains(out, "TsCollSolo_Solo"));
};
