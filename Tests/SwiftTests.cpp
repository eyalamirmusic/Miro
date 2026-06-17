// Tests for Miro::Swift — emits Swift Codable types and a typed Client
// from the same TypeNode tree the TypeScript / C++ renderers consume.
// The types renderer maps objects -> `struct: Codable`, enums ->
// `enum: String, Codable`, and non-identifier JSON keys -> CodingKeys.
// The client renderer mirrors the C++ client (Swift -> C++ direction).

#include "TestHelpers.h"
#include "TestTypes.h"

#include <Miro/Miro.h>
#include <Miro/Swift/Swift.h>
#include <Miro/Swift/SwiftClient.h>
#include <NanoTest/NanoTest.h>

#include <span>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

// A type whose JSON keys are valid identifiers but Swift keywords —
// must be backtick-escaped, and (because Codable strips the backticks
// to derive the key) must NOT force a CodingKeys map.
struct SwiftKeywordFields
{
    void reflect(Miro::Reflector& ref)
    {
        ref["default"](defaultValue);
        ref["case"](caseValue);
    }

    int defaultValue = 0;
    int caseValue = 0;
};

} // namespace

// ---------- Types renderer ----------

auto swStructCodable = test("Swift: object becomes a Codable struct") = []
{
    auto out = Swift::toSource<Address>();
    check(contains(out, "struct Address: Codable {"));
    check(contains(out, "var street: String"));
    check(contains(out, "var zip: String"));
};

auto swPrimitiveTypes =
    test("Swift: primitive fields map to Bool / Int / Double / String") = []
{
    auto out = Swift::toSource<User>();
    check(contains(out, "var name: String"));
    check(contains(out, "var age: Int"));
    check(contains(out, "var active: Bool"));
};

auto swContainerTypes = test("Swift: array / map / optional spellings") = []
{
    auto out = Swift::toSource<User>();
    check(contains(out, "var tags: [String]"));
    check(contains(out, "var counters: [String: Int]"));
    check(contains(out, "var note: String? = nil"));
    check(contains(out, "var shipping: Address? = nil"));
};

auto swNamedReference = test("Swift: nested named struct referenced by name") = []
{
    auto out = Swift::toSource<User>();
    check(contains(out, "var address: Address"));
};

auto swEnumDeclaration =
    test("Swift: enum becomes String-backed Codable with raw values") = []
{
    auto out = Swift::toSource<User>();
    check(contains(out, "enum Color: String, Codable {"));
    check(contains(out, "case Red = \"Red\""));
    check(contains(out, "case Green = \"Green\""));
    check(contains(out, "case Blue = \"Blue\""));
};

auto swDependencyOrder =
    test("Swift: referenced types are declared before their users") = []
{
    auto out = Swift::toSource<User>();
    check(comesBefore(out, "struct Address: Codable", "struct User: Codable"));
    check(comesBefore(out, "enum Color", "struct User: Codable"));
};

auto swInt64Field = test("Swift: 64-bit integer fields render as Int64") = []
{
    auto out = Swift::toSource<ClassWithInt64>();
    check(contains(out, "var epochMs: Int64"));
};

auto swKeywordFieldsBackticked =
    test("Swift: keyword field names are backtick-escaped, no CodingKeys") = []
{
    auto out = Swift::toSource<SwiftKeywordFields>();
    check(contains(out, "var `default`: Int"));
    check(contains(out, "var `case`: Int"));
    // Keys equal the property names (backticks stripped), so no remap.
    check(!contains(out, "CodingKeys"));
};

auto swCodingKeysForNonIdentifier =
    test("Swift: non-identifier JSON keys get a CodingKeys map") = []
{
    auto out = Swift::toSource<NamedMembers>();
    check(contains(out, "var Full_Name: String"));
    check(contains(out, "var Item_Count: Int"));
    check(contains(out, "var price_ratio: Double"));
    check(contains(out, "enum CodingKeys: String, CodingKey {"));
    check(contains(out, "case Full_Name = \"Full Name\""));
    check(contains(out, "case Item_Count = \"Item Count\""));
    check(contains(out, "case price_ratio = \"price-ratio\""));
};

// ---------- Client renderer ----------

namespace
{

struct SCTPingResponse
{
    bool ok = true;

    MIRO_REFLECT(ok)
};

struct SCTEchoRequest
{
    std::string text;

    MIRO_REFLECT(text)
};

struct SCTEchoResponse
{
    std::string echoed;

    MIRO_REFLECT(echoed)
};

CommandExport::CommandEntry makeEntry(std::string name,
                                      bool hasReq,
                                      std::string reqType,
                                      std::string reqQual,
                                      bool hasRes,
                                      std::string resType,
                                      std::string resQual)
{
    auto entry = CommandExport::CommandEntry {};
    entry.name = std::move(name);
    entry.hasRequest = hasReq;
    entry.requestTypeName = std::move(reqType);
    entry.requestQualifiedName = std::move(reqQual);
    entry.hasResponse = hasRes;
    entry.responseTypeName = std::move(resType);
    entry.responseQualifiedName = std::move(resQual);
    return entry;
}

} // namespace

auto sctClientShell =
    test("SwiftClient: emitted file has import, final class, ctor") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    auto entries = std::vector<CommandExport::CommandEntry> {};

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(out, "import Foundation"));
    check(contains(out, "final class Client {"));
    check(contains(out, "typealias Invoke = (_ command: String, _ payload: Data)"));
    check(contains(out, "init(invoke: @escaping Invoke) {"));
};

auto sctEmptyRegistry =
    test("SwiftClient: zero commands -> class with only the ctor") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    auto entries = std::vector<CommandExport::CommandEntry> {};

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(!contains(out, "func "));
};

auto sctResAndReqMethod =
    test("SwiftClient: Res(Req) emits typed param and return") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<SCTEchoRequest>());
    roots.push_back(TypeTree::buildTree<SCTEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("echo",
                  true,
                  std::string {Detail::typeNameOf<SCTEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<SCTEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<SCTEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<SCTEchoResponse>()}),
    };

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(out,
                   "func echo(_ req: SCTEchoRequest) throws -> SCTEchoResponse {"));
    check(contains(out, "let payload = try encoder.encode(req)"));
    check(contains(out, "let result = try invoke(\"echo\", payload)"));
    check(contains(out,
                   "return try decoder.decode(SCTEchoResponse.self, from: result)"));
};

auto sctEmptyRequestElided =
    test("SwiftClient: empty-struct request elides the parameter") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<EmptyValue>());
    roots.push_back(TypeTree::buildTree<SCTPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("ping",
                  true,
                  std::string {Detail::typeNameOf<EmptyValue>()},
                  std::string {Detail::qualifiedNameOf<EmptyValue>()},
                  true,
                  std::string {Detail::typeNameOf<SCTPingResponse>()},
                  std::string {Detail::qualifiedNameOf<SCTPingResponse>()}),
    };

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(out, "func ping() throws -> SCTPingResponse {"));
    check(contains(out, "let payload = Data(\"{}\".utf8)"));
    check(contains(out, "let result = try invoke(\"ping\", payload)"));
};

auto sctResOnlyMethod =
    test("SwiftClient: Res() emits no-arg method calling invoke with empty") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<SCTPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("status",
                  false,
                  "",
                  "",
                  true,
                  std::string {Detail::typeNameOf<SCTPingResponse>()},
                  std::string {Detail::qualifiedNameOf<SCTPingResponse>()}),
    };

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(out, "func status() throws -> SCTPingResponse {"));
    check(contains(out, "let payload = Data(\"{}\".utf8)"));
};

auto sctVoidReqMethod =
    test("SwiftClient: void(Req) returns nothing and discards the result") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<SCTEchoRequest>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("log",
                  true,
                  std::string {Detail::typeNameOf<SCTEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<SCTEchoRequest>()},
                  false,
                  "",
                  ""),
    };

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(out, "func log(_ req: SCTEchoRequest) throws {"));
    check(!contains(out, "func log(_ req: SCTEchoRequest) throws ->"));
    check(contains(out, "_ = try invoke(\"log\", payload)"));
};

auto sctVoidNoArgMethod =
    test("SwiftClient: void() emits no-arg throwing method") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("quit", false, "", "", false, "", ""),
    };

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(out, "func quit() throws {"));
    check(contains(out, "let payload = Data(\"{}\".utf8)"));
    check(contains(out, "_ = try invoke(\"quit\", payload)"));
};

auto sctNamespacedFlattened =
    test("SwiftClient: a.b.name flattens to a_b_name with original wire name") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<SCTEchoRequest>());
    roots.push_back(TypeTree::buildTree<SCTEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("api.v2.echo",
                  true,
                  std::string {Detail::typeNameOf<SCTEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<SCTEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<SCTEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<SCTEchoResponse>()}),
    };

    auto out = Swift::formatClient(std::span<TypeTree::TypeNode> {roots}, entries);

    check(contains(
        out, "func api_v2_echo(_ req: SCTEchoRequest) throws -> SCTEchoResponse {"));
    check(contains(out, "invoke(\"api.v2.echo\", payload)"));
};
