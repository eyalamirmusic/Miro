// Tests for the inversion-driven codegen entrypoint:
//   buildCodegen<Apis...>(baseName, formats) -> EmittedFile list
//
// The test exercises the full pipeline — DescribeReflector walk over
// the listed APIs, format pipeline run against the resulting Context,
// EmittedFile contents inspected directly (no filesystem). Proves the
// new path produces the same shape of TS modules the static-init main
// has always produced, without any MIRO_EXPORT_COMMAND macro.

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <string>

using namespace nano;
using namespace Miro;
using namespace Miro::TypeExport;

namespace
{

struct CMReq
{
    std::string text;

    MIRO_REFLECT(text)
};

struct CMRes
{
    std::string echoed;

    MIRO_REFLECT(echoed)
};

class CMTestApi
{
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void reflect(ApiReflector& r)
    {
        r.command(&CMTestApi::echo, "echo");
        r.command(&CMTestApi::status, "status");
        r.command(&CMTestApi::log, "log");
        r.command(&CMTestApi::quit, "quit");
        r.event(&CMTestApi::changes, "changes");
    }

    CMRes echo(const CMReq& req) { calls++; return CMRes {req.text + "!"}; }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    CMRes status() const { return CMRes {"ok"}; }
    void log(const CMReq& req) { lastLogged = req.text; }
    void quit() { quitCalls++; }

    Event<CMRes> changes;
    std::string lastLogged;
    int calls = 0;
    int quitCalls = 0;
};

const EmittedFile* findFile(const EA::Vector<EmittedFile>& files,
                            std::string_view suffix)
{
    for (auto& f: files)
    {
        if (f.filename.size() >= suffix.size()
            && std::string_view {f.filename}.substr(
                   f.filename.size() - suffix.size())
                == suffix)
            return &f;
    }
    return nullptr;
}

} // namespace

auto cmTypesModule =
    test("codegenMain: ts format emits typed interfaces for all reachable payloads") = []
{
    auto files =
        buildCodegen<CMTestApi>("schema", EA::Vector<std::string> {"ts"});

    auto* ts = findFile(files, ".ts");
    check(ts != nullptr);
    check(ts->contents.find("export interface CMReq") != std::string::npos);
    check(ts->contents.find("export interface CMRes") != std::string::npos);
    check(ts->contents.find("text:") != std::string::npos);
    check(ts->contents.find("echoed:") != std::string::npos);
};

auto cmBackendModule =
    test("codegenMain: backend format emits one typed call per command") = []
{
    auto files =
        buildCodegen<CMTestApi>("schema", EA::Vector<std::string> {"backend"});

    auto* backend = findFile(files, ".backend.ts");
    check(backend != nullptr);

    // All four pmf shapes round-trip into the right TS signatures.
    check(backend->contents.find("echo: (req: T.CMReq): Promise<T.CMRes>")
          != std::string::npos);
    check(backend->contents.find("status: (): Promise<T.CMRes>")
          != std::string::npos);
    check(backend->contents.find("log: (req: T.CMReq): Promise<void>")
          != std::string::npos);
    check(backend->contents.find("quit: (): Promise<void>") != std::string::npos);
};

auto cmHandlersModule =
    test("codegenMain: ts-server format emits Handlers + dispatch") = []
{
    auto files =
        buildCodegen<CMTestApi>("schema", EA::Vector<std::string> {"ts-server"});

    auto* handlers = findFile(files, ".handlers.ts");
    check(handlers != nullptr);

    check(handlers->contents.find("export type Handlers") != std::string::npos);
    check(handlers->contents.find("echo(req: T.CMReq)") != std::string::npos);
    check(handlers->contents.find("case 'echo':") != std::string::npos);
    check(handlers->contents.find("case 'quit':") != std::string::npos);
};

auto cmZodModule =
    test("codegenMain: zod format emits z.object() schemas") = []
{
    auto files =
        buildCodegen<CMTestApi>("schema", EA::Vector<std::string> {"zod"});

    auto* zod = findFile(files, ".zod.ts");
    check(zod != nullptr);
    check(zod->contents.find("import { z } from \"zod\";") != std::string::npos);
    check(zod->contents.find("export const CMReq") != std::string::npos);
    check(zod->contents.find("export const CMRes") != std::string::npos);
};

auto cmDefaultsToAllFormats =
    test("codegenMain: empty formats list runs every registered format") = []
{
    auto files = buildCodegen<CMTestApi>("schema", EA::Vector<std::string> {});

    // Spot-check that the canonical suspects all turned up.
    check(findFile(files, ".ts") != nullptr);
    check(findFile(files, ".backend.ts") != nullptr);
    check(findFile(files, ".zod.ts") != nullptr);
    check(findFile(files, ".bridge.ts") != nullptr);
    check(findFile(files, ".schema.json") != nullptr);
};

auto cmCustomBasename =
    test("codegenMain: baseName threads through to emitted filenames") = []
{
    auto files = buildCodegen<CMTestApi>("api",
                                         EA::Vector<std::string> {"ts", "backend"});

    auto* ts = findFile(files, ".ts");
    check(ts != nullptr);
    check(ts->filename == "api.ts");

    auto* backend = findFile(files, ".backend.ts");
    check(backend != nullptr);
    check(backend->filename == "api.backend.ts");
    // Backend module's import line should reference the new basename.
    check(backend->contents.find("import type * as T from './api';")
          != std::string::npos);
};

auto cmMultipleApis =
    test("codegenMain: parameter pack aggregates commands across APIs") = []
{
    // Second tiny API — distinct command names — to prove pack expansion
    // collects everything into one Context.
    struct CMOtherApi
    {
        // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
        void reflect(ApiReflector& r) { r.command(&CMOtherApi::other, "other"); }
        // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
        CMRes other() const { return CMRes {"other"}; }
    };

    auto files = buildCodegen<CMTestApi, CMOtherApi>(
        "schema", EA::Vector<std::string> {"backend"});

    auto* backend = findFile(files, ".backend.ts");
    check(backend != nullptr);
    check(backend->contents.find("echo:") != std::string::npos);
    check(backend->contents.find("other:") != std::string::npos);
};

auto cmCommandTranslation =
    test("codegenMain: toCommandEntries preserves shape flags + type names") = []
{
    auto describe = ::Miro::Detail::DescribeReflector {};
    auto api = CMTestApi {};
    api.reflect(describe);

    auto entries = toCommandEntries(describe.commands);

    check(entries.size() == 4);

    auto findEntry = [&](const std::string& name)
        -> const CommandExport::CommandEntry*
    {
        for (auto& e: entries)
            if (e.name == name)
                return &e;
        return nullptr;
    };

    auto* echo = findEntry("echo");
    check(echo != nullptr);
    check(echo->hasRequest);
    check(echo->hasResponse);
    check(echo->requestTypeName == "CMReq");
    check(echo->responseTypeName == "CMRes");

    auto* quit = findEntry("quit");
    check(quit != nullptr);
    check(!quit->hasRequest);
    check(!quit->hasResponse);
};
