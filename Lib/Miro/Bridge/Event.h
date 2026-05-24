#pragma once

// Typed event sources for the API-reflection layer.
//
// An event is the in-class declaration of a push-only channel: it owns
// (or refers to) the latest payload and exposes an EA::Broadcaster that
// listeners attach to. Transports bound via Bridge::use(api) subscribe
// to broadcaster() and forward snapshot() over the wire.
//
// Two storage flavours, same shape:
//
//   Event<T>     — owns its own T. The API class calls publish(T) after
//                  each mutation; the event copies the value in and
//                  triggers. Use when the event is the canonical home
//                  of the state.
//
//   RefEvent<T>  — non-owning; constructed with a T& to externally
//                  owned state. snapshot() reads through the ref;
//                  publish() only triggers (the API mutates its own T
//                  directly, then signals). Use when the API class
//                  already holds the state and you don't want a second
//                  copy living in the event.
//
//   class Todos
//   {
//   public:
//       Todos() : changes(state) {}
//
//       void addTodo(const AddRequest& r)
//       {
//           state.items.create(...);
//           changes.publish();   // ref'd state already updated
//       }
//
//       TodoState state;
//       Miro::RefEvent<TodoState> changes;
//   };
//
// Both types satisfy the duck-typed `EventLike` concept below — the
// minimal interface ApiReflector / EventMemberInfo expect. New event
// flavours (e.g. a future shared-state variant) just need to satisfy
// EventLike and add a sibling EventMemberInfo specialization.
//
// Non-copyable / non-movable: listeners hold pointers into our
// broadcaster, and RefEvent additionally holds a pointer into external
// state, so silently relocating either side would break subscriptions
// or dangle. Wrap in a Pimpl / hold by reference if you need to move
// the owning class around.

#include <ea_data_structures/Pointers/Broadcaster.h>

#include <concepts>
#include <type_traits>
#include <utility>

namespace Miro
{

template <typename T>
class Event
{
public:
    Event() = default;
    explicit Event(T initial)
        : value(std::move(initial))
    {
    }

    Event(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;

    const T& snapshot() const { return value; }

    EA::Broadcaster& broadcaster() { return bcast; }

    void publish(T next)
    {
        value = std::move(next);
        bcast.trigger();
    }

    void publish() { bcast.trigger(); }

private:
    T value {};
    EA::Broadcaster bcast;
};

// Non-owning event: holds a reference into externally-owned state. The
// referenced T must outlive this RefEvent — the natural pattern is to
// declare the T member BEFORE the RefEvent member in the same class so
// member-init order constructs the storage first.
template <typename T>
class RefEvent
{
public:
    explicit RefEvent(T& source) noexcept
        : ref(&source)
    {
    }

    RefEvent(const RefEvent&) = delete;
    RefEvent(RefEvent&&) = delete;
    RefEvent& operator=(const RefEvent&) = delete;
    RefEvent& operator=(RefEvent&&) = delete;

    const T& snapshot() const { return *ref; }

    EA::Broadcaster& broadcaster() { return bcast; }

    // Signals that the referenced state has changed. The user updates
    // their owned T directly; this just triggers the broadcaster.
    void publish() { bcast.trigger(); }

private:
    T* ref;
    EA::Broadcaster bcast;
};

// Duck-typed contract every event source must satisfy to be usable by
// the reflector. New event flavours can be plugged in by satisfying
// this concept and adding an EventMemberInfo specialization for them.
template <typename E>
concept EventLike = requires(E& e, const E& ce) {
    { e.broadcaster() } -> std::same_as<EA::Broadcaster&>;
    {
        ce.snapshot()
    }
    -> std::convertible_to<const std::remove_reference_t<decltype(ce.snapshot())>&>;
};

} // namespace Miro
