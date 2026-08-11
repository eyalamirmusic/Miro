#pragma once

// Events are non-copyable / non-movable: attached listeners hold pointers
// into the broadcaster, so relocating one would break live subscriptions.

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

// The referenced T must outlive this RefEvent — declare the T member
// BEFORE the RefEvent member so member-init order constructs it first.
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

    void publish() { bcast.trigger(); }

private:
    T* ref;
    EA::Broadcaster bcast;
};

// A new event flavour must satisfy this and add an EventMemberInfo
// specialization for itself to be usable by the reflector.
template <typename E>
concept EventLike = requires(E& e, const E& ce) {
    { e.broadcaster() } -> std::same_as<EA::Broadcaster&>;
    {
        ce.snapshot()
    }
    -> std::convertible_to<const std::remove_reference_t<decltype(ce.snapshot())>&>;
};

} // namespace Miro
