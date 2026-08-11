#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <span>
#include <string>

using namespace nano;
using namespace Miro;

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

auto cetBackendImports =
    test("CommandExport: backend module imports types by basename") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    auto entries = std::vector<CommandExport::CommandEntry> {};

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");
    check(contains(out, "import type * as T from './schema';"));
    check(contains(out, "export function makeBackend(invoke: Invoke)"));
};

auto cetBackendResAndReq =
    test("CommandExport: Res(Req) emits typed param and return") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETEchoRequest>());
    roots.push_back(TypeTree::buildTree<CETEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("echo",
                  true,
                  std::string {Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CETEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<CETEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<CETEchoResponse>()}),
    };

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(
        contains(out, "echo: (req: T.CETEchoRequest): Promise<T.CETEchoResponse>"));
    check(contains(out, "invoke('echo', req) as Promise<T.CETEchoResponse>"));
};

auto cetBackendEmptyRequestElided =
    test("CommandExport: empty-struct request elides the parameter") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<EmptyValue>());
    roots.push_back(TypeTree::buildTree<CETPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("ping",
                  true,
                  std::string {Detail::typeNameOf<EmptyValue>()},
                  std::string {Detail::qualifiedNameOf<EmptyValue>()},
                  true,
                  std::string {Detail::typeNameOf<CETPingResponse>()},
                  std::string {Detail::qualifiedNameOf<CETPingResponse>()}),
    };

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "ping: (): Promise<T.CETPingResponse>"));
    check(contains(out, "invoke('ping', {}) as Promise<T.CETPingResponse>"));
};

auto cetBackendResOnly = test("CommandExport: Res() emits no-arg signature") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("status",
                  false,
                  "",
                  "",
                  true,
                  std::string {Detail::typeNameOf<CETPingResponse>()},
                  std::string {Detail::qualifiedNameOf<CETPingResponse>()}),
    };

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "status: (): Promise<T.CETPingResponse>"));
};

auto cetBackendVoidReq = test("CommandExport: void(Req) emits Promise<void>") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETEchoRequest>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("log",
                  true,
                  std::string {Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CETEchoRequest>()},
                  false,
                  "",
                  ""),
    };

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "log: (req: T.CETEchoRequest): Promise<void>"));
    check(contains(out, "invoke('log', req) as Promise<void>"));
};

auto cetBackendVoidNoArg =
    test("CommandExport: void() emits no-arg Promise<void>") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("quit", false, "", "", false, "", ""),
    };

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "quit: (): Promise<void>"));
    check(contains(out, "invoke('quit', {}) as Promise<void>"));
};

auto cetBackendNamespaceNests =
    test("CommandExport: a.b.name produces nested objects") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETEchoRequest>());
    roots.push_back(TypeTree::buildTree<CETEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("api.v2.echo",
                  true,
                  std::string {Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CETEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<CETEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<CETEchoResponse>()}),
    };

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "api: {"));
    check(contains(out, "v2: {"));
    check(contains(out, "echo: (req: T.CETEchoRequest)"));
    check(contains(out, "invoke('api.v2.echo', req)"));
};

auto cetHandlersImports =
    test("CommandExport: handlers module imports types and emits scaffolding") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    auto entries = std::vector<CommandExport::CommandEntry> {};

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "import type * as T from './schema';"));
    check(contains(out, "export type Handlers = {"));
    check(contains(out, "export class UnknownCommandError extends Error"));
    check(contains(out,
                   "export async function dispatch(handlers: Handlers, "
                   "command: string, _payload: unknown): Promise<unknown>"));
    check(contains(out, "default: throw new UnknownCommandError(command);"));
};

auto cetHandlersResAndReq = test(
    "CommandExport (handlers): Res(Req) emits typed param and union return") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETEchoRequest>());
    roots.push_back(TypeTree::buildTree<CETEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("echo",
                  true,
                  std::string {Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CETEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<CETEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<CETEchoResponse>()}),
    };

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out,
                   "echo(req: T.CETEchoRequest): T.CETEchoResponse | "
                   "Promise<T.CETEchoResponse>;"));
    check(contains(out,
                   "case 'echo': return await handlers.echo("
                   "payload as T.CETEchoRequest);"));
};

auto cetHandlersEmptyRequestElided =
    test("CommandExport (handlers): empty-struct request elides the parameter") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<EmptyValue>());
    roots.push_back(TypeTree::buildTree<CETPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("ping",
                  true,
                  std::string {Detail::typeNameOf<EmptyValue>()},
                  std::string {Detail::qualifiedNameOf<EmptyValue>()},
                  true,
                  std::string {Detail::typeNameOf<CETPingResponse>()},
                  std::string {Detail::qualifiedNameOf<CETPingResponse>()}),
    };

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "ping(): T.CETPingResponse | Promise<T.CETPingResponse>;"));
    check(contains(out, "case 'ping': return await handlers.ping();"));
};

auto cetHandlersResOnly =
    test("CommandExport (handlers): Res() emits no-arg signature") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("status",
                  false,
                  "",
                  "",
                  true,
                  std::string {Detail::typeNameOf<CETPingResponse>()},
                  std::string {Detail::qualifiedNameOf<CETPingResponse>()}),
    };

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(
        contains(out, "status(): T.CETPingResponse | Promise<T.CETPingResponse>;"));
    check(contains(out, "case 'status': return await handlers.status();"));
};

auto cetHandlersVoidReq =
    test("CommandExport (handlers): void(Req) emits void-union return") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETEchoRequest>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("log",
                  true,
                  std::string {Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CETEchoRequest>()},
                  false,
                  "",
                  ""),
    };

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "log(req: T.CETEchoRequest): void | Promise<void>;"));
    check(contains(out,
                   "case 'log': return await handlers.log("
                   "payload as T.CETEchoRequest);"));
};

auto cetHandlersVoidNoArg =
    test("CommandExport (handlers): void() emits no-arg void union") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("quit", false, "", "", false, "", ""),
    };

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "quit(): void | Promise<void>;"));
    check(contains(out, "case 'quit': return await handlers.quit();"));
};

auto cetHandlersNamespaceNests =
    test("CommandExport (handlers): a.b.name nests in type and dispatch") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CETEchoRequest>());
    roots.push_back(TypeTree::buildTree<CETEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("api.v2.echo",
                  true,
                  std::string {Detail::typeNameOf<CETEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CETEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<CETEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<CETEchoResponse>()}),
    };

    auto out = CommandExport::formatServerHandlersModule(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "api: {"));
    check(contains(out, "v2: {"));
    check(contains(out, "echo(req: T.CETEchoRequest)"));
    check(contains(out,
                   "case 'api.v2.echo': return await handlers.api.v2.echo("
                   "payload as T.CETEchoRequest);"));
};

auto cetBackendCollisionThrows =
    test("CommandExport: same path used as both leaf and namespace throws") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<EmptyValue>());
    roots.push_back(TypeTree::buildTree<CETPingResponse>());

    auto resQual = std::string {Detail::qualifiedNameOf<CETPingResponse>()};
    auto resName = std::string {Detail::typeNameOf<CETPingResponse>()};

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("api", false, "", "", true, resName, resQual),
        makeEntry("api.ping", false, "", "", true, resName, resQual),
    };

    auto threw = false;
    try
    {
        CommandExport::formatBackendModule(
            std::span<TypeTree::TypeNode> {roots}, entries, "schema");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    check(threw);
};
