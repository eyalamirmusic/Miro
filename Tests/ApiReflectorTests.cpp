// End-to-end tests for the ApiReflector inversion: a user-defined API
// class with a reflect() method declares its commands and events; the
// library walks that method polymorphically — Bridge::use(api) binds
// each command into the bridge's CommandTable and each event to a
// Listener that emits the snapshot, while DescribeReflector records
// the same declarations for codegen consumption.
//
// These tests cover the full round-trip:
//   1. dispatching the bridge's command table calls into the API
//      instance's methods
//   2. publishing on a Miro::Event<T> member triggers a bridge.onEmit
//      broadcast carrying the event name + serialized snapshot
//   3. the describe path records identical metadata (names, type
//      identities, req/res shape) without touching the instance

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <ea_data_structures/Pointers/Broadcaster.h>

#include <string>

using namespace nano;
using namespace Miro;

namespace
{

struct ARReq
{
    std::string text;

    MIRO_REFLECT(text)
};

struct ARRes
{
    std::string echoed;

    MIRO_REFLECT(echoed)
};

// One representative class with one of each pmf shape plus an event
// member. All four shape paths through MethodInfo + the event-member
// path through EventMemberInfo are exercised together — if any one
// of them mis-deduces, the corresponding test fails.
class TodosTestApi
{
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void reflect(ApiReflector& r)
    {
        using T = TodosTestApi;
        r.commands<&T::echo, &T::status, &T::log, &T::tick>();
        r.event<&T::changes>();
    }

    ARRes echo(const ARReq& req)
    {
        calls++;
        return ARRes {req.text + "!"};
    }

    ARRes status() const { return ARRes {lastLogged.empty() ? "idle" : "busy"}; }

    void log(const ARReq& req)
    {
        lastLogged = req.text;
        changes.publish(ARRes {"logged:" + req.text});
    }

    void tick()
    {
        ticks++;
        changes.publish(ARRes {"tick:" + std::to_string(ticks)});
    }

    Event<ARRes> changes;
    std::string lastLogged;
    int ticks = 0;
    int calls = 0;
};

// Helper: captures (event-name, payload) on every bridge.onEmit fire.
// Reads the bridge's currentEvent/currentPayload inside the listener
// body, matching how real transports observe emits.
struct EmitCapture
{
    EmitCapture(Bridge& b)
        : listener(
              b.onEmit,
              [this, &b]
              {
                  lastEvent = std::string {b.currentEvent()};
                  lastPayload = b.currentPayload();
              },
              EA::Listener::Modes::TriggerOnEvent)
    {
    }

    std::string lastEvent;
    JSON lastPayload;
    EA::Listener listener;
};

} // namespace

// ---------- Bind mode: command dispatch ----------

auto arBindDispatchesEcho =
    test("ApiReflector: Bridge::use installs Res(Req) handler") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    auto result = bridge.dispatch("echo", Json::parse(R"({"text":"hi"})"));

    check(result["echoed"].asString() == "hi!");
    check(api.calls == 1);
};

auto arBindDispatchesStatus = test(
    "ApiReflector: Bridge::use installs Res() const handler reading instance") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    check(bridge.dispatch("status", JSON {})["echoed"].asString() == "idle");

    api.lastLogged = "x";
    check(bridge.dispatch("status", JSON {})["echoed"].asString() == "busy");
};

auto arBindDispatchesLog = test(
    "ApiReflector: Bridge::use installs void(Req) handler mutating instance") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    auto result = bridge.dispatch("log", Json::parse(R"({"text":"trace"})"));

    check(result.isNull());
    check(api.lastLogged == "trace");
};

auto arBindDispatchesTick =
    test("ApiReflector: Bridge::use installs void() handler mutating instance") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    bridge.dispatch("tick", JSON {});
    bridge.dispatch("tick", JSON {});

    check(api.ticks == 2);
};

// ---------- Bind mode: event emission ----------

auto arBindEventEmitsOnPublish = test(
    "ApiReflector: publishing on an Event<T> member triggers bridge.onEmit") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    auto capture = EmitCapture {bridge};

    api.changes.publish(ARRes {"hello"});

    check(capture.lastEvent == "changes");
    check(capture.lastPayload["echoed"].asString() == "hello");
};

auto arBindEventViaCommand =
    test("ApiReflector: a command that publishes routes through bridge.onEmit") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api);

    auto capture = EmitCapture {bridge};

    bridge.dispatch("log", Json::parse(R"({"text":"abc"})"));

    check(capture.lastEvent == "changes");
    check(capture.lastPayload["echoed"].asString() == "logged:abc");
};

// ---------- Bind mode: listener lifetime ----------

auto arBindListenerSurvivesUseCall =
    test("ApiReflector: subscription outlives the BindReflector itself") = []
{
    auto api = TodosTestApi {};
    auto bridge = Bridge {};
    bridge.use(api); // BindReflector is constructed + destroyed inline here

    auto capture = EmitCapture {bridge};

    api.changes.publish(ARRes {"still alive"});

    check(capture.lastEvent == "changes");
    check(capture.lastPayload["echoed"].asString() == "still alive");
};

// ---------- Describe mode ----------

namespace
{
const Detail::DescribeReflector::CommandRecord*
    findCmd(const EA::Vector<Detail::DescribeReflector::CommandRecord>& cmds,
            const std::string& name)
{
    for (auto& c: cmds)
        if (c.name == name)
            return &c;
    return nullptr;
}

const Detail::DescribeReflector::EventRecord*
    findEvt(const EA::Vector<Detail::DescribeReflector::EventRecord>& events,
            const std::string& name)
{
    for (auto& e: events)
        if (e.name == name)
            return &e;
    return nullptr;
}
} // namespace

auto arDescribeRecordsAllCommands =
    test("ApiReflector: DescribeReflector records every declared command") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    check(describe.commands.size() == 4);
    check(findCmd(describe.commands, "echo") != nullptr);
    check(findCmd(describe.commands, "status") != nullptr);
    check(findCmd(describe.commands, "log") != nullptr);
    check(findCmd(describe.commands, "tick") != nullptr);
};

auto arDescribeCommandShapes =
    test("ApiReflector: DescribeReflector captures shape flags per command") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    auto* echo = findCmd(describe.commands, "echo");
    check(echo != nullptr);
    check(bool(echo->req));
    check(bool(echo->res));
    check(echo->req.name == "ARReq");
    check(echo->res.name == "ARRes");

    auto* status = findCmd(describe.commands, "status");
    check(status != nullptr);
    check(!bool(status->req));
    check(bool(status->res));
    check(status->res.name == "ARRes");

    auto* log = findCmd(describe.commands, "log");
    check(log != nullptr);
    check(bool(log->req));
    check(!bool(log->res));
    check(log->req.name == "ARReq");

    auto* tick = findCmd(describe.commands, "tick");
    check(tick != nullptr);
    check(!bool(tick->req));
    check(!bool(tick->res));
};

auto arDescribeRecordsEvent =
    test("ApiReflector: DescribeReflector records events with payload type") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    check(describe.events.size() == 1);
    auto* changes = findEvt(describe.events, "changes");
    check(changes != nullptr);
    check(changes->payload.name == "ARRes");
};

auto arDescribeDoesNotInvoke = test(
    "ApiReflector: DescribeReflector never invokes handlers on the instance") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    check(api.calls == 0);
    check(api.ticks == 0);
    check(api.lastLogged.empty());
};

// ---------- Describe mode: TypeNode collection ----------

namespace
{
// The test types live in an anonymous namespace, so their qualifiedName
// is "(anonymous namespace)::ARReq" etc. Match on the short typeName
// for readability.
bool hasTypeRoot(const EA::Vector<TypeTree::TypeNode>& roots,
                 std::string_view typeName)
{
    for (auto& r: roots)
        if (r.typeName == typeName)
            return true;
    return false;
}
} // namespace

auto arDescribeBuildsTypeRoots = test(
    "ApiReflector: DescribeReflector builds TypeNodes for every Req/Res/payload") =
    []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    // ARReq appears on echo (Req) and log (Req).
    // ARRes appears on echo (Res), status (Res), and changes (payload).
    check(hasTypeRoot(describe.typeRoots, "ARReq"));
    check(hasTypeRoot(describe.typeRoots, "ARRes"));
};

auto arDescribeTypeRootsAreStructural =
    test("ApiReflector: collected TypeNodes carry the field structure") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    auto findRoot = [&](std::string_view name) -> const TypeTree::TypeNode*
    {
        for (auto& r: describe.typeRoots)
            if (r.typeName == name)
                return &r;
        return nullptr;
    };

    auto* areq = findRoot("ARReq");
    check(areq != nullptr);
    check(areq->shape == TypeTree::TypeNode::Shape::Object);
    check(areq->fields.size() == 1);
    check(areq->fields[0].name == "text");

    auto* ares = findRoot("ARRes");
    check(ares != nullptr);
    check(ares->shape == TypeTree::TypeNode::Shape::Object);
    check(ares->fields.size() == 1);
    check(ares->fields[0].name == "echoed");
};

// ---------- End-to-end: DescribeReflector → existing formatBackendModule ----------
//
// Proves the inversion can drive the existing format pipeline: we
// build a CommandEntry list from DescribeReflector's records and feed
// it (along with the collected type roots) into the same
// formatBackendModule the static-init path uses. If the output text
// has all the typed signatures we'd expect from the test API, the
// reflector-to-formatter integration is wired.

// ---------- NTTP form: explicit-name overload still works for renames ----------
//
// The string overload remains available for cases where the wire name
// must differ from the C++ member name. This class declares both a
// renamed command and a renamed event to confirm the legacy path is
// untouched by the new NTTP defaults.

namespace
{
class RenameTestApi
{
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void reflect(ApiReflector& r)
    {
        r.command(&RenameTestApi::ping, "ping_v2");
        r.event(&RenameTestApi::pulses, "pulse");
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    ARRes ping() const { return ARRes {"pong"}; }

    Event<ARRes> pulses;
};
} // namespace

auto arRenameStringOverloadStillWorks = test(
    "ApiReflector: string overload renames command and event independently") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = RenameTestApi {};
    api.reflect(describe);

    check(describe.commands.size() == 1);
    check(describe.commands[0].name == "ping_v2");
    check(describe.events.size() == 1);
    check(describe.events[0].name == "pulse");
};

// ---------- NTTP form: events<...> variadic ----------
//
// TodosTestApi covers the commands<...> fan-out and the singular event<>
// form; this case verifies that events<...> walks multiple pmds.

namespace
{
class TwoEventsApi
{
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void reflect(ApiReflector& r)
    {
        r.events<&TwoEventsApi::alpha, &TwoEventsApi::beta>();
    }

    Event<ARRes> alpha;
    Event<ARRes> beta;
};
} // namespace

auto arEventsVariadicDerivesNames = test(
    "ApiReflector: events<...> fans out and derives each name from the pmd") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TwoEventsApi {};
    api.reflect(describe);

    check(describe.events.size() == 2);
    check(findEvt(describe.events, "alpha") != nullptr);
    check(findEvt(describe.events, "beta") != nullptr);
};

auto arDescribeDrivesBackendFormat =
    test("ApiReflector: DescribeReflector output feeds formatBackendModule") = []
{
    auto describe = Detail::DescribeReflector {};
    auto api = TodosTestApi {};
    api.reflect(describe);

    auto entries = std::vector<CommandExport::CommandEntry> {};
    for (auto& c: describe.commands)
    {
        auto e = CommandExport::CommandEntry {};
        e.name = c.name;
        e.hasRequest = bool(c.req);
        e.requestTypeName = c.req.name;
        e.requestQualifiedName = c.req.qualifiedName;
        e.hasResponse = bool(c.res);
        e.responseTypeName = c.res.name;
        e.responseQualifiedName = c.res.qualifiedName;
        entries.push_back(std::move(e));
    }

    auto out = CommandExport::formatBackendModule(
        std::span<TypeTree::TypeNode> {
            describe.typeRoots.data(),
            static_cast<std::size_t>(describe.typeRoots.size())},
        entries,
        "schema");

    // Each command shape appears with the right signature.
    check(out.find("echo: (req: T.ARReq): Promise<T.ARRes>") != std::string::npos);
    check(out.find("status: (): Promise<T.ARRes>") != std::string::npos);
    check(out.find("log: (req: T.ARReq): Promise<void>") != std::string::npos);
    check(out.find("tick: (): Promise<void>") != std::string::npos);
};
