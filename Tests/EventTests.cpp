#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <ea_data_structures/Pointers/Broadcaster.h>

#include <string>

using namespace nano;
using namespace Miro;

namespace
{

struct EventTestPayload
{
    int n = 0;
    std::string s {};

    MIRO_REFLECT(n, s)
};

} // namespace

auto evDefaultSnapshot =
    test("Event: default-constructs with value-initialized payload") = []
{
    auto e = Event<EventTestPayload> {};
    check(e.snapshot().n == 0);
    check(e.snapshot().s.empty());
};

auto evInitialPayload =
    test("Event: initial-value constructor seeds the snapshot") = []
{
    auto e = Event<EventTestPayload> {EventTestPayload {.n = 7, .s = "seed"}};
    check(e.snapshot().n == 7);
    check(e.snapshot().s == "seed");
};

auto evPublishUpdatesSnapshot =
    test("Event: publish(payload) updates snapshot before firing listeners") = []
{
    auto e = Event<EventTestPayload> {};

    auto observed = EventTestPayload {};
    auto listener = EA::Listener {e.broadcaster(),
                                  [&] { observed = e.snapshot(); },
                                  EA::Listener::Modes::TriggerOnEvent};

    e.publish({.n = 42, .s = "hello"});

    check(observed.n == 42);
    check(observed.s == "hello");
    check(e.snapshot().n == 42);
};

auto evPublishNoArg =
    test("Event: publish() no-arg fires listener without mutating snapshot") = []
{
    auto e = Event<EventTestPayload> {EventTestPayload {.n = 5}};

    auto fired = 0;
    auto listener = EA::Listener {
        e.broadcaster(), [&] { fired++; }, EA::Listener::Modes::TriggerOnEvent};

    e.publish();
    e.publish();

    check(fired == 2);
    check(e.snapshot().n == 5);
};

auto evMultipleListeners = test("Event: each listener fires on publish") = []
{
    auto e = Event<int> {};

    auto a = 0;
    auto b = 0;

    auto la = EA::Listener {e.broadcaster(),
                            [&] { a = e.snapshot(); },
                            EA::Listener::Modes::TriggerOnEvent};
    auto lb = EA::Listener {e.broadcaster(),
                            [&] { b = e.snapshot() * 2; },
                            EA::Listener::Modes::TriggerOnEvent};

    e.publish(3);

    check(a == 3);
    check(b == 6);
};

auto evListenerDetachOnDestroy = test(
    "Event: listener detaches on destruction - later publish does not fire it") = []
{
    auto e = Event<int> {};

    auto fired = 0;
    {
        auto listener = EA::Listener {
            e.broadcaster(), [&] { fired++; }, EA::Listener::Modes::TriggerOnEvent};
        e.publish(1);
        check(fired == 1);
    }

    e.publish(2);
    check(fired == 1);
};

auto refEvSnapshotReadsThroughRef =
    test("RefEvent: snapshot() reads through the referenced T") = []
{
    auto state = EventTestPayload {.n = 11, .s = "ref-init"};
    auto e = RefEvent<EventTestPayload> {state};

    check(e.snapshot().n == 11);
    check(e.snapshot().s == "ref-init");

    state.n = 99;
    state.s = "mutated";

    // Still current without a publish(): the ref'd T is the only source of truth.
    check(e.snapshot().n == 99);
    check(e.snapshot().s == "mutated");
};

auto refEvPublishFiresWithCurrentRef = test(
    "RefEvent: publish() triggers listeners, which observe the ref'd value") = []
{
    auto state = EventTestPayload {.n = 0};
    auto e = RefEvent<EventTestPayload> {state};

    auto observed = EventTestPayload {};
    auto listener = EA::Listener {e.broadcaster(),
                                  [&] { observed = e.snapshot(); },
                                  EA::Listener::Modes::TriggerOnEvent};

    state.n = 17;
    state.s = "external";
    e.publish();

    check(observed.n == 17);
    check(observed.s == "external");
};

auto refEvDetachOnDestroy = test("RefEvent: listener detaches on destruction") = []
{
    auto state = 0;
    auto e = RefEvent<int> {state};

    auto fired = 0;
    {
        auto listener = EA::Listener {
            e.broadcaster(), [&] { fired++; }, EA::Listener::Modes::TriggerOnEvent};
        state = 1;
        e.publish();
        check(fired == 1);
    }

    state = 2;
    e.publish();
    check(fired == 1);
};

static_assert(EventLike<Event<int>>);
static_assert(EventLike<RefEvent<int>>);
