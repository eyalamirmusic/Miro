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

    MIRO_REFLECT(name, age, active, address, tags, counters, note, shipping)
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
