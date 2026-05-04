// Tests for the CommandExport machinery: the four handler shapes
// (Res(Req) / Res() / void(Req) / void()), namespace nesting in the
// emitted module, empty-request elision, registerCommand thunk
// behaviour, and the static-init registration flow driven by the
// MIRO_EXPORT_COMMAND(S) macros.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <algorithm>
#include <span>
#include <string>

using namespace nano;

namespace
{

struct CETPingResponse
{
    bool ok = true;

    MIRO_REFLECT(ok)
};

struct CETEchoRequest
{
    std::string text;

    MIRO_REFLECT(text)
};

struct CETEchoResponse
{
    std::string echoed;

    MIRO_REFLECT(echoed)
};

// One handler per shape. They're at namespace scope (rather than in
// an anonymous namespace) so MIRO_EXPORT_COMMAND can take their
// address as a non-type template argument and the qualified-name
// stringification matches the JS-side expectation.
CETPingResponse cetWithReqAndRes(const Miro::EmptyValue&)
{
    return CETPingResponse {.ok = true};
}

CETPingResponse cetWithResOnly()
{
    return CETPingResponse {.ok = true};
}

void cetWithReqOnly(const CETEchoRequest&) {}

void cetWithNeither() {}

CETEchoResponse cetEcho(const CETEchoRequest& req)
{
    return CETEchoResponse {.echoed = req.text + "!"};
}

namespace api
{
CETEchoResponse cetNamespaced(const CETEchoRequest& req)
{
    return CETEchoResponse {.echoed = "(api)" + req.text};
}
} // namespace api

// Helpers for hand-building CommandEntry without touching the global
// registry — keeps formatBackendModule tests deterministic regardless
// of static-init order.
Miro::CommandExport::CommandEntry makeEntry(std::string name,
                                            bool hasReq,
                                            std::string reqType,
                                            std::string reqQual,
                                            bool hasRes,
                                            std::string resType,
                                            std::string resQual)
{
    auto entry = Miro::CommandExport::CommandEntry {};
    entry.name = std::move(name);
    entry.hasRequest = hasReq;
    entry.requestTypeName = std::move(reqType);
    entry.requestQualifiedName = std::move(reqQual);
    entry.hasResponse = hasRes;
    entry.responseTypeName = std::move(resType);
    entry.responseQualifiedName = std::move(resQual);
    return entry;
}

const Miro::CommandExport::CommandEntry*
findRegistered(const std::string& name)
{
    auto& reg = Miro::CommandExport::Detail::registry();
    auto it =
        std::find_if(reg.begin(),
                     reg.end(),
                     [&](auto& e) { return e.name == name; });
    return it == reg.end() ? nullptr : &*it;
}

} // namespace

// All four handler shapes are wired up at static-init via the macros,
// so the tests below can look them up by name and exercise their
// thunks directly.
MIRO_EXPORT_COMMANDS(cetWithReqAndRes,
                    cetWithResOnly,
                    cetWithReqOnly,
                    cetWithNeither,
                    cetEcho,
                    api::cetNamespaced)

// ---------- Static-init registration ----------

auto cetMacroRegistersAll =
    test("CommandExport: MIRO_EXPORT_COMMANDS registers each handler") = []
{
    check(findRegistered("cetWithReqAndRes") != nullptr);
    check(findRegistered("cetWithResOnly") != nullptr);
    check(findRegistered("cetWithReqOnly") != nullptr);
    check(findRegistered("cetWithNeither") != nullptr);
    check(findRegistered("cetEcho") != nullptr);
    check(findRegistered("api::cetNamespaced") != nullptr);
};

auto cetSigResAndReq =
    test("CommandExport: Res(Req) records both type identities") = []
{
    auto* e = findRegistered("cetWithReqAndRes");
    check(e != nullptr);
    check(e->hasRequest);
    check(e->hasResponse);
    check(e->requestTypeName == "EmptyValue");
    check(e->responseTypeName == "CETPingResponse");
};

auto cetSigResOnly =
    test("CommandExport: Res() leaves request side empty") = []
{
    auto* e = findRegistered("cetWithResOnly");
    check(e != nullptr);
    check(!e->hasRequest);
    check(e->hasResponse);
    check(e->requestTypeName.empty());
    check(e->responseTypeName == "CETPingResponse");
};

auto cetSigReqOnly =
    test("CommandExport: void(Req) leaves response side empty") = []
{
    auto* e = findRegistered("cetWithReqOnly");
    check(e != nullptr);
    check(e->hasRequest);
    check(!e->hasResponse);
    check(e->requestTypeName == "CETEchoRequest");
    check(e->responseTypeName.empty());
};

auto cetSigNeither =
    test("CommandExport: void() leaves both sides empty") = []
{
    auto* e = findRegistered("cetWithNeither");
    check(e != nullptr);
    check(!e->hasRequest);
    check(!e->hasResponse);
    check(e->requestTypeName.empty());
    check(e->responseTypeName.empty());
};

// ---------- Thunk behaviour ----------

auto cetThunkResAndReq = test("CommandExport: thunk for Res(Req) round-trips") = []
{
    auto* e = findRegistered("cetEcho");
    check(e != nullptr);

    auto payload = Miro::Json::parse(R"({"text":"hi"})");
    auto result = e->thunk(payload);

    check(result.isObject());
    check(result["echoed"].asString() == "hi!");
};

auto cetThunkResOnly = test("CommandExport: thunk for Res() ignores payload") = []
{
    auto* e = findRegistered("cetWithResOnly");
    check(e != nullptr);

    auto result = e->thunk(Miro::Json::Value {});

    check(result.isObject());
    check(result["ok"].asBool() == true);
};

auto cetThunkReqOnly = test("CommandExport: thunk for void(Req) returns null") = []
{
    auto* e = findRegistered("cetWithReqOnly");
    check(e != nullptr);

    auto payload = Miro::Json::parse(R"({"text":"hi"})");
    auto result = e->thunk(payload);

    check(result.isNull());
};

auto cetThunkNeither = test("CommandExport: thunk for void() returns null") = []
{
    auto* e = findRegistered("cetWithNeither");
    check(e != nullptr);

    auto result = e->thunk(Miro::Json::Value {});

    check(result.isNull());
};

// ---------- registerStaticCommandsInto ----------

auto cetWiresIntoTable =
    test("CommandExport: registerStaticCommandsInto exposes each command") = []
{
    auto table = Miro::CommandTable {};
    Miro::CommandExport::registerStaticCommandsInto(table);

    check(table.has("cetEcho"));
    check(table.has("cetWithNeither"));
    check(table.has("api::cetNamespaced"));

    auto payload = Miro::Json::parse(R"({"text":"yo"})");
    auto result = table.dispatch("cetEcho", payload);
    check(result["echoed"].asString() == "yo!");
};

// ---------- formatBackendModule output ----------

auto cetBackendImports =
    test("CommandExport: backend module imports types by basename") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    auto entries = std::vector<Miro::CommandExport::CommandEntry> {};

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");
    check(contains(out, "import type * as T from './schema';"));
    check(contains(out, "export function makeBackend(invoke: Invoke)"));
};

auto cetBackendResAndReq =
    test("CommandExport: Res(Req) emits typed param and return") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    roots.push_back(Miro::TypeTree::buildTree<CETEchoRequest>());
    roots.push_back(Miro::TypeTree::buildTree<CETEchoResponse>());

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("echo",
                  true,
                  std::string {Miro::Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETEchoRequest>()},
                  true,
                  std::string {Miro::Detail::typeNameOf<CETEchoResponse>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETEchoResponse>()}),
    };

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "echo: (req: T.CETEchoRequest): Promise<T.CETEchoResponse>"));
    check(contains(out, "invoke('echo', req) as Promise<T.CETEchoResponse>"));
};

auto cetBackendEmptyRequestElided =
    test("CommandExport: empty-struct request elides the parameter") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    roots.push_back(Miro::TypeTree::buildTree<Miro::EmptyValue>());
    roots.push_back(Miro::TypeTree::buildTree<CETPingResponse>());

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("ping",
                  true,
                  std::string {Miro::Detail::typeNameOf<Miro::EmptyValue>()},
                  std::string {Miro::Detail::qualifiedNameOf<Miro::EmptyValue>()},
                  true,
                  std::string {Miro::Detail::typeNameOf<CETPingResponse>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETPingResponse>()}),
    };

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "ping: (): Promise<T.CETPingResponse>"));
    check(contains(out, "invoke('ping', {}) as Promise<T.CETPingResponse>"));
};

auto cetBackendResOnly =
    test("CommandExport: Res() emits no-arg signature") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    roots.push_back(Miro::TypeTree::buildTree<CETPingResponse>());

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("status",
                  false,
                  "",
                  "",
                  true,
                  std::string {Miro::Detail::typeNameOf<CETPingResponse>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETPingResponse>()}),
    };

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "status: (): Promise<T.CETPingResponse>"));
};

auto cetBackendVoidReq =
    test("CommandExport: void(Req) emits Promise<void>") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    roots.push_back(Miro::TypeTree::buildTree<CETEchoRequest>());

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("log",
                  true,
                  std::string {Miro::Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETEchoRequest>()},
                  false,
                  "",
                  ""),
    };

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "log: (req: T.CETEchoRequest): Promise<void>"));
    check(contains(out, "invoke('log', req) as Promise<void>"));
};

auto cetBackendVoidNoArg =
    test("CommandExport: void() emits no-arg Promise<void>") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("quit", false, "", "", false, "", ""),
    };

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "quit: (): Promise<void>"));
    check(contains(out, "invoke('quit', {}) as Promise<void>"));
};

auto cetBackendNamespaceNests =
    test("CommandExport: a::b::name produces nested objects") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    roots.push_back(Miro::TypeTree::buildTree<CETEchoRequest>());
    roots.push_back(Miro::TypeTree::buildTree<CETEchoResponse>());

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("api::v2::echo",
                  true,
                  std::string {Miro::Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETEchoRequest>()},
                  true,
                  std::string {Miro::Detail::typeNameOf<CETEchoResponse>()},
                  std::string {Miro::Detail::qualifiedNameOf<CETEchoResponse>()}),
    };

    auto out = Miro::CommandExport::formatBackendModule(
        std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "api: {"));
    check(contains(out, "v2: {"));
    check(contains(out, "echo: (req: T.CETEchoRequest)"));
    check(contains(out, "invoke('api::v2::echo', req)"));
};

auto cetBackendCollisionThrows =
    test("CommandExport: same path used as both leaf and namespace throws") = []
{
    auto roots = std::vector<Miro::TypeTree::TypeNode> {};
    roots.push_back(Miro::TypeTree::buildTree<Miro::EmptyValue>());
    roots.push_back(Miro::TypeTree::buildTree<CETPingResponse>());

    auto resQual = std::string {Miro::Detail::qualifiedNameOf<CETPingResponse>()};
    auto resName = std::string {Miro::Detail::typeNameOf<CETPingResponse>()};

    auto entries = std::vector<Miro::CommandExport::CommandEntry> {
        makeEntry("api", false, "", "", true, resName, resQual),
        makeEntry("api::ping", false, "", "", true, resName, resQual),
    };

    auto threw = false;
    try
    {
        Miro::CommandExport::formatBackendModule(
            std::span<Miro::TypeTree::TypeNode> {roots}, entries, "schema");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    check(threw);
};
