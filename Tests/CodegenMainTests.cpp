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
    void reflect(ApiReflector& r)
    {
        r.command(&CMTestApi::echo, "echo");
        r.command(&CMTestApi::status, "status");
        r.command(&CMTestApi::log, "log");
        r.command(&CMTestApi::quit, "quit");
        r.event(&CMTestApi::changes, "changes");
    }

    CMRes echo(const CMReq& req)
    {
        calls++;
        return {req.text + "!"};
    }
    CMRes status() const { return {"ok"}; }
    void log(const CMReq& req) { lastLogged = req.text; }
    void quit() { quitCalls++; }

    Event<CMRes> changes;
    std::string lastLogged;
    int calls = 0;
    int quitCalls = 0;
};

class CMFilesSub
{
public:
    void reflect(ApiReflector& r) { r.event(&CMFilesSub::changed, "changed"); }

    Event<CMRes> changed;
};

class CMCompositeApi
{
public:
    void reflect(ApiReflector& r)
    {
        r.event(&CMCompositeApi::topEvt, "topEvt");
        r.use("files", files);
    }

    Event<CMRes> topEvt;
    CMFilesSub files;
};

const EmittedFile* findFile(const EA::Vector<EmittedFile>& files,
                            std::string_view suffix)
{
    for (auto& f: files)
    {
        if (f.filename.size() >= suffix.size()
            && std::string_view {f.filename}.substr(f.filename.size()
                                                    - suffix.size())
                   == suffix)
            return &f;
    }
    return nullptr;
}

} // namespace

auto cmTypesModule = test(
    "codegenMain: ts format emits typed interfaces for all reachable payloads") = []
{
    auto files = buildCodegen<CMTestApi>("schema", {"ts"});

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
    auto files = buildCodegen<CMTestApi>("schema", {"backend"});

    auto* backend = findFile(files, ".backend.ts");
    check(backend != nullptr);

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
    auto files = buildCodegen<CMTestApi>("schema", {"ts-server"});

    auto* handlers = findFile(files, ".handlers.ts");
    check(handlers != nullptr);

    check(handlers->contents.find("export type Handlers") != std::string::npos);
    check(handlers->contents.find("echo(req: T.CMReq)") != std::string::npos);
    check(handlers->contents.find("case 'echo':") != std::string::npos);
    check(handlers->contents.find("case 'quit':") != std::string::npos);
};

auto cmZodModule = test("codegenMain: zod format emits z.object() schemas") = []
{
    auto files = buildCodegen<CMTestApi>("schema", {"zod"});

    auto* zod = findFile(files, ".zod.ts");
    check(zod != nullptr);
    check(zod->contents.find("import { z } from \"zod\";") != std::string::npos);
    check(zod->contents.find("export const CMReq") != std::string::npos);
    check(zod->contents.find("export const CMRes") != std::string::npos);
};

auto cmDefaultsToAllFormats =
    test("codegenMain: empty formats list runs every registered format") = []
{
    auto files = buildCodegen<CMTestApi>("schema", {});

    check(findFile(files, ".ts") != nullptr);
    check(findFile(files, ".backend.ts") != nullptr);
    check(findFile(files, ".zod.ts") != nullptr);
    check(findFile(files, ".bridge.ts") != nullptr);
    check(findFile(files, ".schema.json") != nullptr);
};

auto cmCustomBasename =
    test("codegenMain: baseName threads through to emitted filenames") = []
{
    auto files = buildCodegen<CMTestApi>("api", {"ts", "backend"});

    auto* ts = findFile(files, ".ts");
    check(ts != nullptr);
    check(ts->filename == "api.ts");

    auto* backend = findFile(files, ".backend.ts");
    check(backend != nullptr);
    check(backend->filename == "api.backend.ts");
    check(backend->contents.find("import type * as T from './api';")
          != std::string::npos);
};

auto cmMultipleApis =
    test("codegenMain: parameter pack aggregates commands across APIs") = []
{
    struct CMOtherApi
    {
        void reflect(ApiReflector& r) { r.command(&CMOtherApi::other, "other"); }
        CMRes other() const { return {"other"}; }
    };

    auto files = buildCodegen<CMTestApi, CMOtherApi>("schema", {"backend"});

    auto* backend = findFile(files, ".backend.ts");
    check(backend != nullptr);
    check(backend->contents.find("echo:") != std::string::npos);
    check(backend->contents.find("other:") != std::string::npos);
};

auto cmEventsModule =
    test("codegenMain: events format emits typed Events + EventBus interfaces") = []
{
    auto files = buildCodegen<CMTestApi>("schema", {"events"});

    auto* events = findFile(files, ".events.ts");
    check(events != nullptr);
    check(events->contents.find("import type * as T from './schema';")
          != std::string::npos);
    check(events->contents.find("export interface Events") != std::string::npos);
    check(events->contents.find("'changes': T.CMRes") != std::string::npos);
    check(events->contents.find("export type EventName = keyof Events;")
          != std::string::npos);
    check(events->contents.find("export interface EventBus") != std::string::npos);
    check(events->contents.find("subscribe<K extends EventName>")
          != std::string::npos);
};

auto cmEventsModuleNoEvents = test(
    "codegenMain: event-less API emits no schema import in its events module") = []
{
    struct CMNoEventsApi
    {
        void reflect(ApiReflector& r) { r.command(&CMNoEventsApi::ping, "ping"); }
        CMRes ping() const { return {"pong"}; }
    };

    auto files = buildCodegen<CMNoEventsApi>("schema", {"events"});

    auto* events = findFile(files, ".events.ts");
    check(events != nullptr);
    check(events->contents.find("import type * as T") == std::string::npos);

    check(events->contents.find("export interface Events") != std::string::npos);
    check(events->contents.find("export type EventName = keyof Events;")
          != std::string::npos);
    check(events->contents.find("export interface EventBus") != std::string::npos);
};

auto cmEventsModuleSubApi = test(
    "codegenMain: events format emits sub-API events under the use() prefix") = []
{
    auto files = buildCodegen<CMCompositeApi>("schema", {"events"});

    auto* events = findFile(files, ".events.ts");
    check(events != nullptr);
    check(events->contents.find("'topEvt': T.CMRes") != std::string::npos);
    check(events->contents.find("'files.changed': T.CMRes") != std::string::npos);

    check(events->contents.find("'changed': T.CMRes") == std::string::npos);
};

auto cmCommandTranslation =
    test("codegenMain: toCommandEntries preserves shape flags + type names") = []
{
    auto describe = ::Miro::Detail::DescribeReflector {};
    auto api = CMTestApi {};
    api.reflect(describe);

    auto entries = toCommandEntries(describe.commands);

    check(entries.size() == 4);

    auto findEntry =
        [&](const std::string& name) -> const CommandExport::CommandEntry*
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
