#include "TestHelpers.h"

#include <Miro/Cpp/CppClient.h>
#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <span>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

namespace
{

struct CCTPingResponse
{
    bool ok = true;

    MIRO_REFLECT(ok)
};

struct CCTEchoRequest
{
    std::string text;

    MIRO_REFLECT(text)
};

struct CCTEchoResponse
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

auto cctClientHeaderShell =
    test("CppClient: emitted header has pragma once, includes, namespace") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    auto entries = std::vector<CommandExport::CommandEntry> {};

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(out.starts_with("#pragma once"));
    check(contains(out, "#include \"schema.miro.h\""));
    check(contains(out, "#include <Miro/Miro.h>"));
    check(contains(out, "namespace MiroClient"));
    check(contains(out, "class Client"));
    check(contains(out, "using Invoke ="));
};

auto cctEmptyRegistryEmptyClass =
    test("CppClient: zero commands -> Client class with only the ctor") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    auto entries = std::vector<CommandExport::CommandEntry> {};

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "explicit Client(Invoke"));
    check(!contains(out, "::CCT"));
};

auto cctResAndReqMethod =
    test("CppClient: Res(Req) emits typed param and return") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CCTEchoRequest>());
    roots.push_back(TypeTree::buildTree<CCTEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("echo",
                  true,
                  std::string {Detail::typeNameOf<CCTEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CCTEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<CCTEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<CCTEchoResponse>()}),
    };

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "::CCTEchoResponse echo(const ::CCTEchoRequest& req)"));
    check(contains(out, "auto payload = ::Miro::toJSON(req);"));
    check(contains(out, "auto result = invoker(\"echo\", payload);"));
    check(contains(out, "::Miro::fromJSON(out, result);"));
};

auto cctEmptyRequestElided =
    test("CppClient: empty-struct request elides the parameter") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<EmptyValue>());
    roots.push_back(TypeTree::buildTree<CCTPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("ping",
                  true,
                  std::string {Detail::typeNameOf<EmptyValue>()},
                  std::string {Detail::qualifiedNameOf<EmptyValue>()},
                  true,
                  std::string {Detail::typeNameOf<CCTPingResponse>()},
                  std::string {Detail::qualifiedNameOf<CCTPingResponse>()}),
    };

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "::CCTPingResponse ping()"));
    check(contains(out, "::Miro::JSON {::Miro::Json::Object {}}"));
    check(contains(out, "auto result = invoker(\"ping\", payload);"));
};

auto cctResOnlyMethod =
    test("CppClient: Res() emits no-arg method that calls invoke with empty") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CCTPingResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("status",
                  false,
                  "",
                  "",
                  true,
                  std::string {Detail::typeNameOf<CCTPingResponse>()},
                  std::string {Detail::qualifiedNameOf<CCTPingResponse>()}),
    };

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "::CCTPingResponse status()"));
    check(contains(out, "::Miro::JSON {::Miro::Json::Object {}}"));
};

auto cctVoidReqMethod =
    test("CppClient: void(Req) returns void and discards the result") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CCTEchoRequest>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("log",
                  true,
                  std::string {Detail::typeNameOf<CCTEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CCTEchoRequest>()},
                  false,
                  "",
                  ""),
    };

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "void log(const ::CCTEchoRequest& req)"));
    check(contains(out, "(void) invoker(\"log\", payload);"));
};

auto cctVoidNoArgMethod = test("CppClient: void() emits no-arg void method") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("quit", false, "", "", false, "", ""),
    };

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "void quit()"));
    check(contains(out, "(void) invoker(\"quit\", payload);"));
};

auto cctNamespacedFlattened =
    test("CppClient: a.b.name flattens to a_b_name with original wire name") = []
{
    auto roots = std::vector<TypeTree::TypeNode> {};
    roots.push_back(TypeTree::buildTree<CCTEchoRequest>());
    roots.push_back(TypeTree::buildTree<CCTEchoResponse>());

    auto entries = std::vector<CommandExport::CommandEntry> {
        makeEntry("api.v2.echo",
                  true,
                  std::string {Detail::typeNameOf<CCTEchoRequest>()},
                  std::string {Detail::qualifiedNameOf<CCTEchoRequest>()},
                  true,
                  std::string {Detail::typeNameOf<CCTEchoResponse>()},
                  std::string {Detail::qualifiedNameOf<CCTEchoResponse>()}),
    };

    auto out = Cpp::formatClientHeader(
        std::span<TypeTree::TypeNode> {roots}, entries, "schema");

    check(contains(out, "::CCTEchoResponse api_v2_echo(const ::CCTEchoRequest&"));
    check(contains(out, "invoker(\"api.v2.echo\", payload);"));
};
